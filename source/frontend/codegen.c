#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "symtable.h"

/*
 * Vue d'ensemble : trois fonctions a connaitre pour lire ce fichier.
 *
 *   - ecrire_expression(noeud, f) traduit une expression. Elle ECRIT au
 *     passage toutes les instructions intermediaires necessaires sur f
 *     (ex: "_temp_3 = a + b;"), et RENVOIE le nom de la variable/temporaire
 *     qui contient le resultat final (ex: "_temp_3"). C'est ce nom que
 *     l'appelant utilise pour continuer a construire la ligne en cours.
 *
 *   - ecrire_instruction(noeud, f) traduit une instruction (if/while/for/
 *     return/bloc/...). Contrairement a ecrire_expression, elle ne renvoie
 *     rien : une instruction ne produit jamais de valeur utilisable.
 *
 *   - ecrire_condition(cond, lbl, jump_if_true, f) traduit UNE condition en
 *     un saut conditionnel : "if (cond) goto Llbl;" si jump_if_true vaut 1,
 *     ou son inverse ("if (!cond) goto Llbl;", reecrit avec l'operateur
 *     oppose puisque le backend n'a pas de '!') si jump_if_true vaut 0.
 *     C'est elle qui gere && et || en les decomposant en plusieurs sauts
 *     (court-circuit), puisque le backend n'autorise pas les operateurs
 *     booleens dans une condition (cf enonce 4.1).
 *
 * g_expr_sname est un canal de communication global entre ces fonctions :
 * ecrire_expression() le met a jour pour indiquer "le resultat que je
 * viens de renvoyer est un pointeur sur telle structure" (ou NULL si ce
 * n'est pas pertinent). C'est ainsi que "a->suivant->champ" arrive a
 * retrouver l'offset du second "->" sans avoir besoin de relire le type
 * de a depuis la table des symboles a chaque etape.
 */

static int g_temp_compteur  = 0;              // Nombre total de temporaires alloués
static int g_label_compteur = 0;              // Compteur de labels
static int g_indentation    = 0;              // Niveau d'indent
static Symbol *g_local      = NULL;           // Table des symboles locales (empruntee a table_globale)

// Types/struct-name des temporaires : tableaux dynamiques (realloc), pas de
// plafond arbitraire. Une fonction avec une tres grosse expression (des
// centaines d'operateurs) ne doit jamais produire de code reference a un
// temporaire jamais declare.
static char **g_temp_type     = NULL;
static char **g_temp_sname    = NULL;
static int    g_temp_capacite = 0;

// Type/struct-name du resultat de la DERNIERE expression ecrite (cf vue
// d'ensemble ci-dessus). Mis a jour par chaque branche de ecrire_expression
// (toujours, y compris a NULL : pas de valeur residuelle d'un appel
// precedent qui traine par erreur).
static const char *g_expr_sname = NULL;

// Labels en attente d'etre prefixes a la prochaine instruction reellement
// ecrite (cf marquer_label). Tableau dynamique pour la meme raison : une
// longue chaine de "else if" peut accumuler arbitrairement plus de labels
// en attente que n'importe quelle constante fixe choisie a l'avance.
static int *g_pending_labels   = NULL;
static int  g_pending_count    = 0;
static int  g_pending_capacite = 0;

// Met un label en attente : il sera préfixé à la prochaine instruction
// réellement écrite, au lieu d'occuper une ligne "Lx:;" à lui seul.
static void marquer_label(int lbl) {
    if (g_pending_count == g_pending_capacite) {
        g_pending_capacite = g_pending_capacite ? g_pending_capacite * 2 : 8;
        g_pending_labels = realloc(g_pending_labels, sizeof(int) * g_pending_capacite);
    }
    g_pending_labels[g_pending_count++] = lbl;
}

void ecrire_indentation(FILE *f) {
    for (int i = 0; i < g_indentation; i++) fputc('\t', f);
    for (int i = 0; i < g_pending_count; i++) fprintf(f, "L%d: ", g_pending_labels[i]);
    g_pending_count = 0;
}

// A appeler en fin de fonction : si des labels restent en attente (rien ne
// les suit), il faut bien les matérialiser avec une instruction vide.
static void purger_labels_en_attente(FILE *f) {
    if (g_pending_count == 0) return;
    ecrire_indentation(f);
    fprintf(f, ";\n");
}

// Crée un nouveau temporaire (un par sous-expression, jamais réutilisé :
// pas de logique de reutilisation/liberation, donc pas de risque qu'un
// temporaire encore "vivant" soit ecrase par erreur ; le compromis est un
// nombre de temporaires plus eleve que strictement necessaire).
static char *creer_temp(const char *type, const char *sname) {
    int n = g_temp_compteur;
    if (n == g_temp_capacite) {
        g_temp_capacite = g_temp_capacite ? g_temp_capacite * 2 : 32;
        g_temp_type  = realloc(g_temp_type,  sizeof(char *) * g_temp_capacite);
        g_temp_sname = realloc(g_temp_sname, sizeof(char *) * g_temp_capacite);
    }
    g_temp_type [n] = strdup(type);
    g_temp_sname[n] = sname ? strdup(sname) : NULL;
    g_temp_compteur++;
    char *buf = malloc(16);
    snprintf(buf, 16, "_temp_%d", n);
    return buf;
}

// Crée label suivant (Li)
static int creer_label(void) { return ++g_label_compteur; }

// Le token CONSTANT du backend n'inclut pas le signe ('-' est un token a
// part) : un "-N" replie par l'optimisation des constantes negatives
// (cf AST_UNARY) n'est donc PAS un primary_expression valide pour le
// backend. Toute regle de structbe.y qui exige un primary_expression de
// part et d'autre (operandes de +-*/ , arguments d'appel, comparaisons,
// affectation via *ptr = ...) doit donc passer ses valeurs par cette
// fonction avant de les ecrire, afin de materialiser "-N" dans un
// temporaire ("t = -N;") plutot que de l'inserer tel quel.
static char *garantir_primaire(char *val, FILE *f) {
    if (val && val[0] == '-' && val[1] != '_') {
        char *t = creer_temp("int", NULL);
        ecrire_indentation(f); fprintf(f, "%s = %s;\n", t, val);
        free(val);
        return t;
    }
    return val;
}

// Recherche une variable : locales d'abord (g_local, propre a la fonction
// en cours de generation), puis globales (table_globale) si non trouvee.
static Symbol *chercher_variable(const char *name) {
    Symbol *s = g_local  ? chercher_symbole_enfant(g_local,  (char *)name) : NULL;
    if (!s) s = table_globale ? chercher_symbole_enfant(table_globale, (char *)name) : NULL;
    return s;
}

// Recherche une struct (dans table_globale, par nom)
static Symbol *chercher_struct(const char *name) {
    if (!table_globale || !name) return NULL;
    for (int i = 0; i < table_globale->child_count; i++) {
        Symbol *s = table_globale->children[i];
        if (s->type == STRUCT_SYMBOL && s->id && strcmp(s->id, name) == 0)
            return s;
    }
    return NULL;
}

// Retourne la taille d'un struct (default 4)
static int obtenir_taille_struct(const char *sname) {
    Symbol *s = chercher_struct(sname);
    return s ? s->size : 4;
}

// Donne l'offset d'un champ dans un struct et son struct_name si c'est un
// pointeur sur une autre structure (necessaire pour enchainer un second
// "->" sans repasser par la table des symboles, cf g_expr_sname).
static int obtenir_offset_champ(const char *sname, const char *field, char **fsname_out) {
    if (fsname_out) *fsname_out = NULL;
    Symbol *s = chercher_struct(sname);
    if (!s) return 0;
    for (int i = 0; i < s->child_count; i++) {
        if (s->children[i]->id && strcmp(s->children[i]->id, field) == 0) {
            if (fsname_out) *fsname_out = s->children[i]->struct_name;
            return s->children[i]->offset;
        }
    }
    return 0;
}

// Donne le struct_name d'une variable OU d'un temporaire (g_temp_sname,
// indexe par le numero extrait du nom "_temp_N" — c'est le seul endroit
// du fichier qui a besoin de "deviner" l'origine d'un nom a partir de son
// prefixe textuel plutot que de suivre g_expr_sname directement).
static const char *nom_struct_variable(const char *var) {
    if (!var) return NULL;
    if (strncmp(var, "_temp_", 6) == 0) {
        int idx = atoi(var + 6);
        if (idx >= 0 && idx < g_temp_compteur)
            return g_temp_sname[idx];
    }
    Symbol *s = chercher_variable(var);
    return s ? s->struct_name : NULL;
}

// Conversion Ast_node => type C (avec pointeurs et structs). Purement
// base sur la forme du declarateur (ts/decl viennent de l'AST), jamais
// sur la table des symboles : utilisee pour ecrire les signatures
// (parametres, type de retour, variables globales), pas pour le corps.
static const char *chaine_type(Ast_node *ts, Ast_node *decl) {
    if (ts->type == AST_STRUCT) return "void *";  // struct traités comme ptr
    if (ts->type == AST_TYPE_SPECIFIER) {
        if (strcmp(ts->id, "int")  == 0) return ast_est_pointeur(decl) ? "void *" : "int";
        if (strcmp(ts->id, "void") == 0) return ast_est_pointeur(decl) ? "void *" : "void";
    }
    return "void *";
}

// Donne le nom d'un déclarateur
static char *nom_declarateur(Ast_node *decl) {
    Ast_node *id = ast_nom_declarateur(decl);
    return id ? id->id : "?";
}

// Écris proprement les paramètres de func dans le fichier
static void ecrire_parametres(Ast_node *plist, FILE *f) {
    if (!plist) { fprintf(f, "void"); return; }
    int first = 1;
    for (int i = 0; i < plist->children_count; i++) {
        Ast_node *p = plist->children[i];
        if (p->type != AST_PARAM || p->children_count < 2) continue;
        if (!first) fprintf(f, ", ");
        first = 0;
        fprintf(f, "%s %s",
                chaine_type(p->children[0], p->children[1]),
                nom_declarateur(p->children[1]));
    }
    if (first) fprintf(f, "void");
}

// Operateur de comparaison inverse : utilise pour traduire "if (!cond)
// goto L" en un saut equivalent sans negation explicite (le backend n'a
// pas d'operateur '!').
char *inverser_operateur(char *op) {
    if (strcmp(op, "<")  == 0) return ">=";
    if (strcmp(op, "<=") == 0) return ">";
    if (strcmp(op, ">")  == 0) return "<=";
    if (strcmp(op, ">=") == 0) return "<";
    if (strcmp(op, "==") == 0) return "!=";
    if (strcmp(op, "!=") == 0) return "==";
    return "!=";
}

static char *ecrire_expression(Ast_node *nd, FILE *f);
static void ecrire_instruction(Ast_node *nd, FILE *f);

// Traduit une condition en saut conditionnel "if (...) goto Llbl;" (voir
// la vue d'ensemble en tete de fichier pour le role de jump_if_true).
// Trois cas : une comparaison directe (AST_BOOL_OP), un && / || (qu'il
// faut decomposer en plusieurs sauts pour obtenir le court-circuit), ou
// n'importe quelle autre expression (traitee comme "if (v != 0)").
static void ecrire_condition(Ast_node *cond, int lbl, int jump_if_true, FILE *f) {
    if (!cond) return;

    if (cond->type == AST_BOOL_OP && cond->children_count >= 2) {
        char *l = garantir_primaire(ecrire_expression(cond->children[0], f), f);
        char *r = garantir_primaire(ecrire_expression(cond->children[1], f), f);
        const char *op = jump_if_true ? cond->id : inverser_operateur(cond->id);
        ecrire_indentation(f);
        fprintf(f, "if (%s %s %s) goto L%d;\n", l, op, r, lbl);
        free(l); free(r);
        return;
    }

    if (cond->type == AST_BOOL_LOGIC && cond->children_count >= 2) {
        // && et || sont duaux par les lois de De Morgan : sauter quand
        // "a && b" est VRAI revient a sauter quand a ET b sont vrais
        // (deux conditions successives vers le meme label) ; sauter
        // quand "a && b" est FAUX revient a sauter dès que a est faux,
        // SINON evaluer b (d'ou le label 'skip' intermediaire). Le cas
        // || est exactement l'inverse. C'est ce qui produit le
        // court-circuit (b n'est jamais evalue si a suffit a decider).
        if (strcmp(cond->id, "&&") == 0) {
            if (jump_if_true) {
                int skip = creer_label();
                ecrire_condition(cond->children[0], skip, 0, f);
                ecrire_condition(cond->children[1], lbl,  1, f);
                marquer_label(skip);
            } else {
                ecrire_condition(cond->children[0], lbl, 0, f);
                ecrire_condition(cond->children[1], lbl, 0, f);
            }
        } else {
            if (jump_if_true) {
                ecrire_condition(cond->children[0], lbl, 1, f);
                ecrire_condition(cond->children[1], lbl, 1, f);
            } else {
                int skip = creer_label();
                ecrire_condition(cond->children[0], skip, 1, f);
                ecrire_condition(cond->children[1], lbl,  0, f);
                marquer_label(skip);
            }
        }
        return;
    }

    // Cas general : n'importe quelle expression utilisee comme condition
    // (ex: un simple booleen materialise par AST_BOOL_OP/AST_BOOL_LOGIC
    // plus haut dans l'arbre, ou une variable utilisee directement comme
    // condition) est comparee explicitement a 0.
    char *v = ecrire_expression(cond, f);
    ecrire_indentation(f);
    fprintf(f, "if (%s %s 0) goto L%d;\n", v, jump_if_true ? "!=" : "==", lbl);
    free(v);
}

// Ecrit une expression dans le fichier et renvoie le nom de la var résultat
// (cf vue d'ensemble en tete de fichier). Chaque branche doit mettre a
// jour g_expr_sname avant de retourner, meme pour dire "NULL, pas une
// struct" : sinon une valeur laissee par l'appel precedent pourrait être
// lue par erreur par l'appelant.
static char *ecrire_expression(Ast_node *nd, FILE *f) {
    if (!nd) { g_expr_sname = NULL; return strdup("0"); }

    switch (nd->type) {

    case AST_IDENTIFIER: {   // variable ou fonction
        g_expr_sname = NULL;
        Symbol *s = chercher_variable(nd->id);
        if (s) g_expr_sname = s->struct_name;
        return strdup(nd->id);
    }

    case AST_CONSTANT: {     // nombre entier
        g_expr_sname = NULL;
        char *buf = malloc(32);
        snprintf(buf, 32, "%d", nd->value);
        return buf;
    }

    case AST_OP: {     // opération binaire : évalue toujours gauche puis droite
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        char *op = nd->id;
        Ast_node *left  = nd->children[0];
        Ast_node *right = nd->children[1];

        // optimisation sur les neutres : x+0, 0+x, x-0 (evite un
        // temporaire et une ligne "t = x + 0;" totalement inutiles)
        if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
            if (right->type == AST_CONSTANT && right->value == 0)
                return ecrire_expression(left, f);
            if (strcmp(op, "+") == 0 && left->type == AST_CONSTANT && left->value == 0)
                return ecrire_expression(right, f);
        }

        char *l = garantir_primaire(ecrire_expression(left, f), f);
        char *r = garantir_primaire(ecrire_expression(right, f), f);
        char *t = creer_temp("int", NULL);
        ecrire_indentation(f);
        fprintf(f, "%s = %s %s %s;\n", t, l, op, r);
        free(l); free(r);
        g_expr_sname = NULL;
        return t;
    }

    // Comparaison ou && / || utilisées comme valeur: 0/1 via goto !!!
    case AST_BOOL_OP:
    case AST_BOOL_LOGIC: {
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        int tl = creer_label(), fl = creer_label();
        char *t = creer_temp("int", NULL);
        ecrire_condition(nd, tl, 1, f);
        ecrire_indentation(f); fprintf(f, "%s = 0;\n", t);
        ecrire_indentation(f); fprintf(f, "goto L%d;\n", fl);
        ecrire_indentation(f); fprintf(f, "L%d: %s = 1;\n", tl, t);
        marquer_label(fl);
        g_expr_sname = NULL;
        return t;
    }

    case AST_UNARY: {   // unaire (-, &, *)
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        char *op = nd->children[0]->id;
        Ast_node *operand = nd->children[1];

        if (strcmp(op, "-") == 0) {
            /* Optimisation : -constante -> directement "-N" sans temporaire */
            if (operand->type == AST_CONSTANT) {
                g_expr_sname = NULL;
                char *buf = malloc(32);
                snprintf(buf, 32, "-%d", operand->value);
                return buf;
            }
            char *v = ecrire_expression(operand, f);
            char *t = creer_temp("int", NULL);
            ecrire_indentation(f); fprintf(f, "%s = -%s;\n", t, v);
            free(v);
            g_expr_sname = NULL;
            return t;
        }
        if (strcmp(op, "&") == 0) {
            char *v = ecrire_expression(operand, f);
            char *t = creer_temp("void *", NULL);
            ecrire_indentation(f); fprintf(f, "%s = &%s;\n", t, v);
            /* Ne pas libérer v : t contient son adresse, réutiliser v serait incorrect */
            free(v);
            g_expr_sname = NULL;
            return t;
        }
        if (strcmp(op, "*") == 0) {
            char *v = ecrire_expression(operand, f);
            // sn = struct_name de l'OPERANDE (le pointeur dereference),
            // pas du resultat : c'est ce que le temporaire cree doit
            // porter pour qu'un futur "->" sur ce temporaire fonctionne.
            const char *sn = g_expr_sname;
            char *t = creer_temp("void *", sn);
            ecrire_indentation(f); fprintf(f, "%s = *%s;\n", t, v);
            free(v);
            g_expr_sname = sn ? sn : NULL;
            return t;
        }
        g_expr_sname = NULL;
        return strdup("0");
    }

    case AST_UNARY_SIZEOF: {   // sizeof(expr)
        g_expr_sname = NULL;
        if (nd->children_count == 0) return strdup("4");
        Ast_node *arg = nd->children[0];
        const char *sn = NULL;
        if (arg->type == AST_IDENTIFIER) {
            Symbol *s = chercher_variable(arg->id);
            // sizeof(p) = taille de la structure pointée par p
            if (s) sn = s->struct_name;
        }
        int sz = sn ? obtenir_taille_struct(sn) : 4;
        char *buf = malloc(16);
        snprintf(buf, 16, "%d", sz);
        return buf;
    }

    case AST_POSTFIX_POINTER: {     // champ depuis ptr : "p->champ"
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        char *ptr = ecrire_expression(nd->children[0], f);
        // sn : nom de la structure pointee par 'ptr'. D'abord via
        // g_expr_sname (le cas normal, mis a jour par l'appel juste
        // au-dessus), sinon en dernier recours via nom_struct_variable
        // (utile si ptr est redescendu par un chemin qui n'a pas mis a
        // jour g_expr_sname, ex: un parametre de fonction).
        const char *sn = g_expr_sname ? g_expr_sname : nom_struct_variable(ptr);
        char *field = nd->children[1]->id;
        char *fsname = NULL;
        int off = sn ? obtenir_offset_champ(sn, field, &fsname) : 0;

        // Traduction en deux temps, conforme aux affectations autorisees
        // par le backend (cf enonce 4.1, "x = y op z" puis "x = *y") :
        // d'abord calculer l'adresse du champ (pointeur + offset), puis
        // dereferencer cette adresse.
        char *addr = creer_temp("void *", NULL);
        ecrire_indentation(f); fprintf(f, "%s = %s + %d;\n", addr, ptr, off);

        char *val = creer_temp("void *", fsname);
        ecrire_indentation(f); fprintf(f, "%s = *%s;\n", val, addr);

        free(addr);
        free(ptr);
        g_expr_sname = fsname;
        return val;
    }

    case AST_POSTFIX: {      // appel de fonction utilisé comme valeur
        if (nd->children_count == 0) { g_expr_sname = NULL; return strdup("0"); }
        Ast_node *fn_node = nd->children[0];
        char *fname = ecrire_expression(fn_node, f);

        char **args = NULL;
        int argc = 0;
        if (nd->children_count >= 2 &&
            nd->children[1]->type == AST_ARGUMENT_EXPRESSION_LIST) {
            Ast_node *al = nd->children[1];
            args = malloc(sizeof(char *) * al->children_count);
            for (int i = 0; i < al->children_count; i++)
                args[i] = garantir_primaire(ecrire_expression(al->children[i], f), f);
            argc = al->children_count;
        }

        // Fonction void ou retour non utilisé : appel direct sans temp
        // (une fonction "void *" n'est PAS void : il faut exclure les
        // retours pointeur, sinon malloc() etc. perdraient leur valeur)
        Symbol *fn_sym = chercher_variable(fname);
        int returns_void = fn_sym && !fn_sym->pointer &&
                           fn_sym->type_name &&
                           strcmp(fn_sym->type_name, "void") == 0;

        // Détermine le type de retour
        const char *ret_type = "int";
        if (fn_sym) {
            if (fn_sym->pointer) ret_type = "void *";
            else if (fn_sym->type_name) ret_type = fn_sym->type_name;
        }

        ecrire_indentation(f);
        char *t = NULL;
        if (!returns_void) {
            t = creer_temp(ret_type, NULL);
            fprintf(f, "%s = %s(", t, fname);
        } else {
            fprintf(f, "%s(", fname);
        }
        for (int i = 0; i < argc; i++) {
            if (i) fprintf(f, ", ");
            fprintf(f, "%s", args[i]);
        }
        fprintf(f, ");\n");
        for (int i = 0; i < argc; i++) free(args[i]);
        free(args);
        free(fname);
        g_expr_sname = NULL;
        if (!returns_void) return t;
        return strdup("0");
    }

    case AST_ASSIGNMENT: {   // affectation : trois formes selon la cible
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        Ast_node *lhs = nd->children[0];
        Ast_node *rhs = nd->children[1];

        char *rval = ecrire_expression(rhs, f);
        /* sn_rhs est un pointeur non-propriétaire vers la table de symboles */
        const char *sn_rhs = g_expr_sname;

        // Forme 1 : "x = valeur;" — cible simple.
        if (lhs->type == AST_IDENTIFIER) {
            ecrire_indentation(f);
            fprintf(f, "%s = %s;\n", lhs->id, rval);
            // Inference de type a la volee : si on assigne a x une
            // valeur dont on sait que c'est un pointeur sur telle
            // structure, et que x n'a pas encore cette info (ex: un
            // parametre void* generique), on la lui attribue. Utile pour
            // les fonctions comme "applique" qui manipulent des
            // pointeurs de structure via un parametre void*.
            if (sn_rhs && g_local) {
                Symbol *vs = chercher_symbole_enfant(g_local, lhs->id);
                if (vs && !vs->struct_name) vs->struct_name = strdup(sn_rhs);
            }
            g_expr_sname = sn_rhs;
            char *ret = strdup(lhs->id);
            free(rval);
            return ret;
        }

        // Forme 2 : "*p = valeur;" — affectation via dereferencement.
        if (lhs->type == AST_UNARY && lhs->children_count >= 2 &&
            lhs->children[0]->id &&
            strcmp(lhs->children[0]->id, "*") == 0) {
            char *addr = ecrire_expression(lhs->children[1], f);
            rval = garantir_primaire(rval, f);
            ecrire_indentation(f);
            fprintf(f, "*%s = %s;\n", addr, rval);
            free(addr);
            g_expr_sname = NULL;
            char *ret = strdup(rval);
            free(rval);
            return ret;
        }

        // Forme 3 : "p->champ = valeur;" — meme principe que la lecture
        // d'un champ (AST_POSTFIX_POINTER plus haut) : calculer l'adresse
        // du champ, puis ecrire a travers cette adresse.
        if (lhs->type == AST_POSTFIX_POINTER && lhs->children_count >= 2) {
            char *ptr = ecrire_expression(lhs->children[0], f);
            const char *sn = g_expr_sname ? g_expr_sname : nom_struct_variable(ptr);
            char *field = lhs->children[1]->id;
            char *fsname = NULL;
            int off = sn ? obtenir_offset_champ(sn, field, &fsname) : 0;
            char *addr = creer_temp("void *", NULL);
            rval = garantir_primaire(rval, f);
            ecrire_indentation(f); fprintf(f, "%s = %s + %d;\n", addr, ptr, off);
            ecrire_indentation(f); fprintf(f, "*%s = %s;\n", addr, rval);
            free(addr);
            free(ptr);
            g_expr_sname = NULL;
            char *ret = strdup(rval);
            free(rval);
            return ret;
        }

        g_expr_sname = NULL;
        return rval;
    }

    default:
        g_expr_sname = NULL;
        return strdup("0");
    }
}

// Traduit une instruction. Ne renvoie rien (contrairement a
// ecrire_expression) : une instruction ne produit jamais de valeur.
static void ecrire_instruction(Ast_node *nd, FILE *f) {
    if (!nd) return;

    switch (nd->type) {

    case AST_EXPRESSION_STATEMENT:
        if (nd->children_count > 0) {
            Ast_node *expr = nd->children[0];
            /* Appel de fonction comme instruction : on ecrit directement sans temporaire de retour */
            if (expr->type == AST_POSTFIX && expr->children_count >= 1 &&
                (expr->children_count == 1 ||
                 (expr->children_count >= 2 &&
                  expr->children[1]->type == AST_ARGUMENT_EXPRESSION_LIST))) {
                Ast_node *fn_node = expr->children[0];
                char *fname = strdup(fn_node->type == AST_IDENTIFIER ? fn_node->id : "");
                char **args = NULL;
                int argc = 0;
                if (expr->children_count >= 2 &&
                    expr->children[1]->type == AST_ARGUMENT_EXPRESSION_LIST) {
                    Ast_node *al = expr->children[1];
                    args = malloc(sizeof(char *) * al->children_count);
                    for (int i = 0; i < al->children_count; i++)
                        args[i] = garantir_primaire(ecrire_expression(al->children[i], f), f);
                    argc = al->children_count;
                }
                ecrire_indentation(f);
                fprintf(f, "%s(", fname);
                for (int i = 0; i < argc; i++) {
                    if (i) fprintf(f, ", ");
                    fprintf(f, "%s", args[i]);
                }
                fprintf(f, ");\n");
                for (int i = 0; i < argc; i++) free(args[i]);
                free(args);
                free(fname);
            } else {
                // Expression-instruction quelconque (ex: une affectation
                // utilisee comme instruction) : on l'evalue pour son
                // effet de bord et on jette le resultat.
                char *v = ecrire_expression(expr, f);
                free(v);
            }
        }
        break;

    // Le code trois-adresses du backend est entierement aplati via
    // labels/goto : une liste d'instructions ou un bloc { } se traduisent
    // tous les deux par la simple concatenation des instructions filles.
    // (Les declarations locales, elles, n'ont rien a generer ici : elles
    // sont deja toutes regroupees et ecrites en une fois au debut de la
    // fonction, cf ecrire_fonction plus bas.)
    case AST_STATEMENT_LIST:
    case AST_COMPOUND_STATEMENT:
        for (int i = 0; i < nd->children_count; i++)
            ecrire_instruction(nd->children[i], f);
        break;

    case AST_DECLARATION:
        break;

    case AST_IF: {
        // if (cond) corps;
        // se traduit en : if (!cond) goto Lend; corps; Lend:;
        if (nd->children_count < 2) break;
        int lend = creer_label();
        ecrire_condition(nd->children[0], lend, 0, f);
        ecrire_instruction(nd->children[1], f);
        marquer_label(lend);
        break;
    }

    case AST_IF_ELSE: {
        // if (cond) corps1; else corps2;
        // se traduit en : if (!cond) goto Lelse; corps1; goto Lend;
        //                 Lelse:; corps2; Lend:;
        if (nd->children_count < 3) break;
        int lelse = creer_label(), lend = creer_label();
        ecrire_condition(nd->children[0], lelse, 0, f);
        ecrire_instruction(nd->children[1], f);
        ecrire_indentation(f); fprintf(f, "goto L%d;\n", lend);
        marquer_label(lelse);
        ecrire_instruction(nd->children[2], f);
        marquer_label(lend);
        break;
    }

    case AST_WHILE: {
        // while (cond) corps;
        // se traduit en : goto Ltest; Lloop:; corps; Ltest:; if (cond) goto Lloop;
        // (le test est place APRES le corps pour eviter de l'evaluer
        // deux fois par iteration ; le "goto Ltest" initial s'assure
        // qu'il est tout de meme teste avant la premiere iteration).
        if (nd->children_count < 2) break;
        int ltest = creer_label(), lloop = creer_label();
        ecrire_indentation(f); fprintf(f, "goto L%d;\n", ltest);
        marquer_label(lloop);
        ecrire_instruction(nd->children[1], f);
        marquer_label(ltest);
        ecrire_condition(nd->children[0], lloop, 1, f);
        break;
    }

    case AST_FOR: {  // boucle for : enfants = [init, test, increment, corps]
        // Meme schema que while, avec l'init avant le premier goto et
        // l'increment juste avant le test (donc execute a chaque tour,
        // y compris le dernier, avant que le test n'echoue).
        if (nd->children_count < 4) break;
        int ltest = creer_label(), lfor = creer_label();
        ecrire_instruction(nd->children[0], f);
        ecrire_indentation(f); fprintf(f, "goto L%d;\n", ltest);
        marquer_label(lfor);
        ecrire_instruction(nd->children[3], f);
        {   char *v = ecrire_expression(nd->children[2], f); free(v); }
        marquer_label(ltest);
        if (nd->children[1]->children_count > 0)
            ecrire_condition(nd->children[1]->children[0], lfor, 1, f);
        break;
    }

    case AST_RETURN:
        if (nd->children_count > 0) {
            char *v = ecrire_expression(nd->children[0], f);
            ecrire_indentation(f); fprintf(f, "return %s;\n", v);
            free(v);
        } else {
            ecrire_indentation(f); fprintf(f, "return;\n");
        }
        break;

    default:
        for (int i = 0; i < nd->children_count; i++)
            ecrire_instruction(nd->children[i], f);
        break;
    }
}

// Ecrit une fonction en entier : signature, declarations de variables
// (locales + temporaires), puis corps.
static void ecrire_fonction(Ast_node *nd, FILE *f) {
    if (nd->children_count < 3) return;
    Ast_node *ts   = nd->children[0];
    Ast_node *decl = nd->children[1];
    Ast_node *body = nd->children[2];

    // La table des locales (parametres + declarations) a deja ete construite
    // et verifiee par l'analyse semantique : on la reutilise telle quelle.
    Ast_node *plist = ast_liste_parametres(decl);
    Symbol *fs = chercher_symbole_enfant(table_globale, nom_declarateur(decl));
    g_local = fs ? fs->locales : NULL;

    // Reinitialise le compteur de temporaires pour CETTE fonction (les
    // tableaux/leur capacite sont conserves d'une fonction a l'autre,
    // seul le contenu logique est remis a zero : evite de re-allouer a
    // chaque fonction si une precedente en a deja eu beaucoup).
    for (int i = 0; i < g_temp_compteur; i++) {
        free(g_temp_type[i]);  g_temp_type[i]  = NULL;
        free(g_temp_sname[i]); g_temp_sname[i] = NULL;
    }
    g_temp_compteur = 0;
    g_pending_count = 0;

    // Le corps est d'abord ecrit dans un buffer memoire plutot que
    // directement dans f, car le nombre EXACT de temporaires necessaires
    // (donc les lignes "int _temp_N;" a ecrire) n'est connu qu'APRES avoir
    // traduit tout le corps. open_memstream permet d'ecrire le corps une
    // seule fois, puis de le recopier tel quel apres les declarations.
    char *body_buf = NULL;
    size_t body_size = 0;
    FILE *body_f = open_memstream(&body_buf, &body_size);
    g_indentation = 1;
    for (int i = 0; i < body->children_count; i++)
        ecrire_instruction(body->children[i], body_f);
    purger_labels_en_attente(body_f);
    fclose(body_f);

    fprintf(f, "%s %s(", chaine_type(ts, decl), nom_declarateur(decl));
    ecrire_parametres(plist, f);
    fprintf(f, ")\n{\n");

    // Variables locales (hors parametres, deja dans la signature
    // ci-dessus) : on parcourt g_local et on exclut tout symbole dont le
    // nom correspond a un parametre.
    for (int i = 0; g_local && i < g_local->child_count; i++) {
        Symbol *vs = g_local->children[i];
        int is_param = 0;
        if (plist) {
            for (int j = 0; j < plist->children_count && !is_param; j++) {
                Ast_node *p = plist->children[j];
                if (p->children_count >= 2 &&
                    strcmp(nom_declarateur(p->children[1]), vs->id) == 0)
                    is_param = 1;
            }
        }
        if (is_param) continue;
        const char *vt = (vs->pointer ||
                          (vs->type_name && strcmp(vs->type_name, "struct") == 0))
                         ? "void *"
                         : (vs->type_name ? vs->type_name : "int");
        fprintf(f, "\t%s %s;\n", vt, vs->id);
    }

    // Déclarer les variables temp (maintenant qu'on connait leur nombre
    // exact, cf le commentaire sur body_buf plus haut)
    for (int i = 0; i < g_temp_compteur; i++) {
        const char *tt = g_temp_type[i] ? g_temp_type[i] : "int";
        fprintf(f, "\t%s _temp_%d;\n", tt, i);
    }

    if (body_buf) { fputs(body_buf, f); free(body_buf); }
    fprintf(f, "}\n");

    // g_local appartient a table_globale (construit par l'analyse semantique) :
    // on ne le libere pas ici, sem_liberer() s'en chargera.
    g_local = NULL;
}

// Ecrit une declaration extern (fonction ou variable) : signature seule,
// pas de corps.
static void ecrire_extern(Ast_node *nd, FILE *f) {
    if (nd->children_count < 2) return;
    Ast_node *ts   = nd->children[0];
    Ast_node *decl = nd->children[1];
    Ast_node *plist = ast_liste_parametres(decl);
    const char *rt  = chaine_type(ts, decl);
    if (plist || ast_decl_fonction(decl)) {
        fprintf(f, "extern %s %s(", rt, nom_declarateur(decl));
        ecrire_parametres(plist, f);
        fprintf(f, ");\n");
    } else {
        fprintf(f, "extern %s %s;\n", rt, nom_declarateur(decl));
    }
}

// Ecrit une déclaration de var globale
static void ecrire_decl_globale(Ast_node *nd, FILE *f) {
    if (nd->children_count < 2) return;
    fprintf(f, "%s %s;\n",
            chaine_type(nd->children[0], nd->children[1]),
            nom_declarateur(nd->children[1]));
}

// Point d'entree de la generation de code (cf codegen.h). Parcourt
// l'AST de haut niveau une fois et delegue chaque cas a la fonction
// d'ecriture correspondante.
void write_code(Ast_node *prog, FILE *f) {
    if (!prog) return;
    g_label_compteur = 0;
    g_indentation = 0;
    g_local  = NULL;

    // La table globale (variables, fonctions, structs) a deja ete construite
    // et verifiee par l'analyse semantique (table_globale) : pas de second
    // parcours de l'AST pour la reconstruire ici.

    for (int i = 0; i < prog->children_count; i++) {
        Ast_node *nd = prog->children[i];
        switch (nd->type) {
        case AST_STRUCT_DEFINITION: break;  // deja pris en compte par semantic.c (table_globale)
        case AST_EXTERN_DECLARATION: ecrire_extern(nd, f); break;
        case AST_DECLARATION:        ecrire_decl_globale(nd, f); break;
        case AST_FUNCTION_DEFINITION:
            fprintf(f, "\n");
            ecrire_fonction(nd, f);
            break;
        default: break;
        }
    }
}

// Libere les tableaux internes a ce fichier (temporaires, labels en
// attente). table_globale n'est PAS liberee ici : ce fichier ne fait que
// la lire, c'est symtable_liberer() (appele via sem_liberer()) qui en est
// responsable.
void codegen_liberer(void) {
    for (int i = 0; i < g_temp_compteur; i++) {
        free(g_temp_type[i]);
        free(g_temp_sname[i]);
    }
    free(g_temp_type);  g_temp_type  = NULL;
    free(g_temp_sname); g_temp_sname = NULL;
    g_temp_compteur = 0;
    g_temp_capacite = 0;

    free(g_pending_labels); g_pending_labels = NULL;
    g_pending_count = 0;
    g_pending_capacite = 0;
}
