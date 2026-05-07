#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "code.h"

static int g_temp_compteur  = 0;              // Compteur de variables temp
static int g_label_compteur = 0;              // Compteur de labels
static int g_indentation      = 0;              // Niveau d'indent 
static Symbol *g_global  = NULL;           // Table des symboles / noms (var, functions, struct) globales
static Symbol *g_local   = NULL;           // Table des sybmoles locale (incluant param fonctions)

#define MAX_TEMPS 512
 
static char *g_temp_type [MAX_TEMPS];      // Types des variables temp
static char *g_temp_sname[MAX_TEMPS];      // Nom de struct des temp

static const char *g_expr_sname = NULL;    // Nom de struct de la dernière expr évaluée

// Indente dans le fichier (un \t)
void ecrire_indentation(FILE *f) {
    for (int i = 0; i < g_indentation; i++) fputc('\t', f);
}

// Crée variable temp (_ti) avec son type et struct_name
static char *creer_temp(const char *type, const char *sname) {
    int n = ++g_temp_compteur;
    if (n - 1 < MAX_TEMPS) {
        g_temp_type [n-1] = strdup(type);
        g_temp_sname[n-1] = sname ? strdup(sname) : NULL;
    }
    char *buf = malloc(16);
    snprintf(buf, 16, "_t%d", n);
    return buf;
}

// Crée label suivant (Li)
static int creer_label(void) { return ++g_label_compteur; }

// Recherche une variable
static Symbol *chercher_variable(const char *name) {
    Symbol *s = g_local  ? chercher_symbole_enfant(g_local,  (char *)name) : NULL;
    if (!s) s = g_global ? chercher_symbole_enfant(g_global, (char *)name) : NULL;
    return s;
}

// Recherche une struct (dans g_global, par nom)
static Symbol *chercher_struct(const char *name) {
    if (!g_global || !name) return NULL;
    for (int i = 0; i < g_global->child_count; i++) {
        Symbol *s = g_global->children[i];
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

// Donne l'offset d'un champ dans un struct et son struct_name si c'est un pointeur
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

// Donne le struct_name d'une variable
static const char *nom_struct_variable(const char *var) {
    if (!var) return NULL;
    if (var[0] == '_' && var[1] == 't') {  // si temp, lookup g_temp_sname
        int idx = atoi(var + 2) - 1;
        if (idx >= 0 && idx < g_temp_compteur && idx < MAX_TEMPS)
            return g_temp_sname[idx];
    }
    Symbol *s = chercher_variable(var); // sinon lookup g_local puis g_global
    return s ? s->struct_name : NULL;
}

// Déclarateur est un pointeur? (recursive)
static int a_etoile(Ast_node *decl) {
    if (!decl) return 0;
    if (decl->type == AST_STAR_DECLARATOR) return 1;
    for (int i = 0; i < decl->children_count; i++)
        if (a_etoile(decl->children[i])) return 1;
    return 0;
}

// Conversion Ast_node => type C (avec pointeurs et structs)
static const char *chaine_type(Ast_node *ts, Ast_node *decl) {
    if (ts->type == AST_STRUCT) return "void *";  // struct traités comme ptr
    if (ts->type == AST_TYPE_SPECIFIER) {
        if (strcmp(ts->id, "int")  == 0) return a_etoile(decl) ? "void *" : "int";
        if (strcmp(ts->id, "void") == 0) return a_etoile(decl) ? "void *" : "void";
    }
    return "void *";
}

// Donne le nom d'un déclarateur (recursive)
static char *nom_declarateur(Ast_node *decl) {
    if (!decl) return "?";
    if (decl->type == AST_IDENTIFIER) return decl->id;
    if (decl->children_count > 0) return nom_declarateur(decl->children[0]);
    return "?";
}

// Trouve la déclaration de func dans un déclarateur (recursive)
// (on cherche AST_FUNC_DECLARATOR ou AST_DIRECT_DECLARATOR)
static Ast_node *trouver_decl_fonction(Ast_node *decl) {
    if (!decl) return NULL;
    if (decl->type == AST_FUNC_DECLARATOR ||
        decl->type == AST_DIRECT_DECLARATOR) return decl;
    if (decl->children_count > 0)
        return trouver_decl_fonction(decl->children[0]);
    return NULL;
}

// Donne les paramètres à partir d'une déclaration de func
static Ast_node *obtenir_liste_params(Ast_node *decl) {
    Ast_node *fd = trouver_decl_fonction(decl);
    if (fd && fd->type == AST_FUNC_DECLARATOR && fd->children_count >= 2)
        return fd->children[1];
    return NULL;
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

// Helper (pour if !cond goto lbl par exemple)
char *inverser_operateur(char *op) {
    if (strcmp(op, "<")  == 0) return ">=";
    if (strcmp(op, "<=") == 0) return ">";
    if (strcmp(op, ">")  == 0) return "<=";
    if (strcmp(op, ">=") == 0) return "<";
    if (strcmp(op, "==") == 0) return "!=";
    if (strcmp(op, "!=") == 0) return "==";
    return "!=";
}

// Récupère variables, fonctions et structs globales 
static void analyser_programme(Ast_node *prog) {
    g_global = creer_symbole("__global__", 0, IDENTIFIER_SYMBOL);

    for (int i = 0; i < prog->children_count; i++) {
        Ast_node *nd = prog->children[i];
        if (nd->type != AST_STRUCT_DEFINITION || nd->children_count < 2) continue;
        char *sname  = nd->children[0]->id;
        Ast_node *fl = nd->children[1];
        Symbol *ss   = creer_symbole(sname, 0, STRUCT_SYMBOL);
        int off = 0;
        for (int j = 0; j < fl->children_count; j++) {
            Ast_node *fld = fl->children[j];
            if (fld->type != AST_STRUCT_FIELD || fld->children_count < 2) continue;
            Symbol *fs = creer_symbole(nom_declarateur(fld->children[1]), 4, IDENTIFIER_SYMBOL);
            fs->offset = off;
            off += 4;
            if (fld->children[0]->type == AST_STRUCT) {
                fs->pointer = true;
                if (fld->children[0]->children_count > 0)
                    fs->struct_name = strdup(fld->children[0]->children[0]->id);
            }
            if (fld->children[1]->type == AST_STAR_DECLARATOR) fs->pointer = true;
            ajouter_symbole_enfant(ss, fs);
        }
        ss->size = off;
        ajouter_symbole_enfant(g_global, ss);
    }

    for (int i = 0; i < prog->children_count; i++) {
        Ast_node *nd = prog->children[i];
        if (nd->type != AST_DECLARATION &&
            nd->type != AST_EXTERN_DECLARATION &&
            nd->type != AST_FUNCTION_DEFINITION) continue;
        Ast_node *ts   = nd->children[0];
        Ast_node *decl = nd->children[1];
        Symbol_type st = (nd->type == AST_FUNCTION_DEFINITION)
                         ? FUNCTION_SYMBOL : IDENTIFIER_SYMBOL;
        Symbol *gs = creer_symbole(nom_declarateur(decl), 4, st);
        if (ts->type == AST_STRUCT) {
            gs->pointer = true;
            if (ts->children_count > 0)
                gs->struct_name = strdup(ts->children[0]->id);
        }
        if (a_etoile(decl)) gs->pointer = true;
        if (ts->type == AST_TYPE_SPECIFIER) {
            // top_star = la fonction retourne un pointeur (ex: void *malloc(...))
            int top_star = (decl && decl->type == AST_STAR_DECLARATOR);
            if (!top_star) gs->type_name = strdup(ts->id);
        }
        ajouter_symbole_enfant(g_global, gs);
    }
}

// Récupère variables locales (incl. paramètres)
static void collecter_locales(Ast_node *nd, Symbol *scope) {
    if (!nd) return;
    if (nd->type == AST_DECLARATION && nd->children_count >= 2) {
        Ast_node *ts   = nd->children[0];
        Ast_node *decl = nd->children[1];
        char *nm = nom_declarateur(decl);
        if (chercher_symbole_enfant(scope, nm)) return;
        Symbol *vs = creer_symbole(nm, 4, IDENTIFIER_SYMBOL);
        if (ts->type == AST_STRUCT) {
            vs->pointer = true;
            if (ts->children_count > 0)
                vs->struct_name = strdup(ts->children[0]->id);
        } else if (ts->type == AST_TYPE_SPECIFIER) {
            vs->type_name = strdup(ts->id);
        }
        if (decl->type == AST_STAR_DECLARATOR) vs->pointer = true;
        ajouter_symbole_enfant(scope, vs);
        return;
    }
    for (int i = 0; i < nd->children_count; i++)
        collecter_locales(nd->children[i], scope);
}

static char *ecrire_expression(Ast_node *nd, FILE *f);
static void ecrire_instruction(Ast_node *nd, FILE *f);

static int su_label(Ast_node *n) {
    if (!n) return 0;
    switch (n->type) {
    case AST_IDENTIFIER:
    case AST_CONSTANT:
    case AST_UNARY_SIZEOF:
        return 1;
    case AST_OP:
    case AST_BOOL_OP:
    case AST_BOOL_LOGIC: {
        if (n->children_count < 2) return 1;
        int ll = su_label(n->children[0]);
        int lr = su_label(n->children[1]);
        if (ll == lr) return ll + 1;
        return ll > lr ? ll : lr;
    }
    case AST_UNARY:
        if (n->children_count >= 2) return su_label(n->children[1]);
        return 1;
    default:
        return 1;
    }
}

// écrit les conditions avec des goto dans le fichier (if, while, for
static void ecrire_condition(Ast_node *cond, int lbl, int jump_if_true, FILE *f) {
    if (!cond) return;

    if (cond->type == AST_BOOL_OP && cond->children_count >= 2) {
        char *l = ecrire_expression(cond->children[0], f);
        char *r = ecrire_expression(cond->children[1], f);
        const char *op = jump_if_true ? cond->id : inverser_operateur(cond->id);
        ecrire_indentation(f);
        fprintf(f, "if (%s %s %s) goto L%d;\n", l, op, r, lbl);
        free(l); free(r);
        return;
    }

    if (cond->type == AST_BOOL_LOGIC && cond->children_count >= 2) {
        if (strcmp(cond->id, "&&") == 0) {
            if (jump_if_true) {
                int skip = creer_label();
                ecrire_condition(cond->children[0], skip, 0, f);
                ecrire_condition(cond->children[1], lbl,  1, f);
                ecrire_indentation(f); fprintf(f, "L%d:;\n", skip);
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
                ecrire_indentation(f); fprintf(f, "L%d:;\n", skip);
            }
        }
        return;
    }

    char *v = ecrire_expression(cond, f);
    ecrire_indentation(f);
    fprintf(f, "if (%s %s 0) goto L%d;\n", v, jump_if_true ? "!=" : "==", lbl);
    free(v);
}

// Ecrit une expression dans le fichier et renvoie le nom de la var résultat
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

    case AST_OP: {     // opération binaire (Sethi-Ullman: évalue la sous-expr la plus lourde en premier)
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        int sl = su_label(nd->children[0]);
        int sr = su_label(nd->children[1]);
        char *l, *r;
        if (sr > sl) {
            r = ecrire_expression(nd->children[1], f);
            l = ecrire_expression(nd->children[0], f);
        } else {
            l = ecrire_expression(nd->children[0], f);
            r = ecrire_expression(nd->children[1], f);
        }
        char *t = creer_temp("int", NULL);
        ecrire_indentation(f);
        fprintf(f, "%s = %s %s %s;\n", t, l, nd->id, r);
        free(l); free(r);
        g_expr_sname = NULL;
        return t;
    }

    case AST_BOOL_OP: {  // comparaisons (Sethi-Ullman)
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        int sl = su_label(nd->children[0]);
        int sr = su_label(nd->children[1]);
        char *l, *r;
        if (sr > sl) {
            r = ecrire_expression(nd->children[1], f);
            l = ecrire_expression(nd->children[0], f);
        } else {
            l = ecrire_expression(nd->children[0], f);
            r = ecrire_expression(nd->children[1], f);
        }
        char *t = creer_temp("int", NULL);
        ecrire_indentation(f);
        fprintf(f, "%s = (%s %s %s) ? 1 : 0;\n", t, l, nd->id, r);
        free(l); free(r);
        g_expr_sname = NULL;
        return t;
    }

    case AST_BOOL_LOGIC: {   // et / ou logique
        int tl = creer_label(), fl = creer_label();
        char *t = creer_temp("int", NULL);
        ecrire_condition(nd, tl, 1, f);
        ecrire_indentation(f); fprintf(f, "%s = 0;\n", t);
        ecrire_indentation(f); fprintf(f, "goto L%d;\n", fl);
        ecrire_indentation(f); fprintf(f, "L%d: %s = 1;\n", tl, t);
        ecrire_indentation(f); fprintf(f, "L%d:;\n", fl);
        g_expr_sname = NULL;
        return t;
    }

    case AST_UNARY: {   // unaire (-, &, *)
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        char *op = nd->children[0]->id;
        Ast_node *operand = nd->children[1];

        if (strcmp(op, "-") == 0) {
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
            free(v);
            g_expr_sname = NULL;
            return t;
        }
        if (strcmp(op, "*") == 0) {
            char *v = ecrire_expression(operand, f);
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
            // sizeof(pointeur) = 4 toujours ; sizeof(struct) = taille réelle
            if (s && !s->pointer) sn = s->struct_name;
        }
        int sz = sn ? obtenir_taille_struct(sn) : 4;
        char *buf = malloc(16);
        snprintf(buf, 16, "%d", sz);
        return buf;
    }

    case AST_POSTFIX_POINTER: {     // champ depuis ptr
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        char *ptr = ecrire_expression(nd->children[0], f);
        const char *sn = g_expr_sname ? g_expr_sname : nom_struct_variable(ptr);
        char *field = nd->children[1]->id;
        char *fsname = NULL;
        int off = sn ? obtenir_offset_champ(sn, field, &fsname) : 0;

        char *addr = creer_temp("void *", NULL);
        ecrire_indentation(f); fprintf(f, "%s = %s + %d;\n", addr, ptr, off);

        char *val = creer_temp(fsname ? "void *" : "void *", fsname);
        ecrire_indentation(f); fprintf(f, "%s = *%s;\n", val, addr);

        free(ptr);
        g_expr_sname = fsname;
        return val;
    }

    case AST_POSTFIX: {      // appel de fonction
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
                args[i] = ecrire_expression(al->children[i], f);
            argc = al->children_count;
        }

        // Fonction void : pas de temp, appel direct
        // type_name == "void" seulement si le retour est void pur (pas void *)
        Symbol *fn_sym = chercher_variable(fname);
        int returns_void = fn_sym &&
                           fn_sym->type_name &&
                           strcmp(fn_sym->type_name, "void") == 0;

        ecrire_indentation(f);
        if (returns_void) {
            fprintf(f, "%s(", fname);
        } else {
            char *t = creer_temp("void *", NULL);
            fprintf(f, "%s = %s(", t, fname);
            for (int i = 0; i < argc; i++) {
                if (i) fprintf(f, ", ");
                fprintf(f, "%s", args[i]);
            }
            fprintf(f, ");\n");
            for (int i = 0; i < argc; i++) free(args[i]);
            free(args);
            free(fname);
            g_expr_sname = NULL;
            return t;
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
        return strdup("0");
    }

    case AST_ASSIGNMENT: {   // affectation
        if (nd->children_count < 2) { g_expr_sname = NULL; return strdup("0"); }
        Ast_node *lhs = nd->children[0];
        Ast_node *rhs = nd->children[1];

        char *rval = ecrire_expression(rhs, f);
        char *sn_rhs = g_expr_sname ? strdup(g_expr_sname) : NULL;

        if (lhs->type == AST_IDENTIFIER) {
            ecrire_indentation(f);
            fprintf(f, "%s = %s;\n", lhs->id, rval);
            if (sn_rhs && g_local) {
                Symbol *vs = chercher_symbole_enfant(g_local, lhs->id);
                if (vs && !vs->struct_name) vs->struct_name = strdup(sn_rhs);
            }
            g_expr_sname = sn_rhs;
            char *ret = strdup(lhs->id);
            free(rval);
            return ret;
        }

        if (lhs->type == AST_UNARY && lhs->children_count >= 2 &&
            lhs->children[0]->id &&
            strcmp(lhs->children[0]->id, "*") == 0) {
            char *addr = ecrire_expression(lhs->children[1], f);
            ecrire_indentation(f);
            fprintf(f, "*%s = %s;\n", addr, rval);
            free(addr);
            g_expr_sname = NULL;
            char *ret = strdup(rval);
            free(rval); free(sn_rhs);
            return ret;
        }

        if (lhs->type == AST_POSTFIX_POINTER && lhs->children_count >= 2) {
            char *ptr = ecrire_expression(lhs->children[0], f);
            const char *sn = g_expr_sname ? g_expr_sname : nom_struct_variable(ptr);
            char *field = lhs->children[1]->id;
            char *fsname = NULL;
            int off = sn ? obtenir_offset_champ(sn, field, &fsname) : 0;
            char *addr = creer_temp("void *", NULL);
            ecrire_indentation(f); fprintf(f, "%s = %s + %d;\n", addr, ptr, off);
            ecrire_indentation(f); fprintf(f, "*%s = %s;\n", addr, rval);
            free(ptr);
            g_expr_sname = NULL;
            char *ret = strdup(rval);
            free(rval); free(sn_rhs);
            return ret;
        }

        g_expr_sname = NULL;
        free(sn_rhs);
        return rval;
    }

    default:
        g_expr_sname = NULL;
        return strdup("0");
    }
}

static void ecrire_instruction(Ast_node *nd, FILE *f) {
    if (!nd) return;

    switch (nd->type) {

    case AST_EXPRESSION_STATEMENT: 
        if (nd->children_count > 0) {
            char *v = ecrire_expression(nd->children[0], f);
            free(v);
        }
        break;

    case AST_STATEMENT_LIST: 
        for (int i = 0; i < nd->children_count; i++)
            ecrire_instruction(nd->children[i], f);
        break;

    case AST_COMPOUND_STATEMENT:
        ecrire_indentation(f); fprintf(f, "{\n");
        g_indentation++;
        for (int i = 0; i < nd->children_count; i++)
            ecrire_instruction(nd->children[i], f);
        g_indentation--;
        ecrire_indentation(f); fprintf(f, "}\n");
        break;

    case AST_DECLARATION:
        break;

    case AST_IF: {
        if (nd->children_count < 2) break;
        int lend = creer_label();
        ecrire_condition(nd->children[0], lend, 0, f);
        ecrire_instruction(nd->children[1], f);
        ecrire_indentation(f); fprintf(f, "L%d:;\n", lend);
        break;
    }

    case AST_IF_ELSE: {  // if-else
        if (nd->children_count < 3) break;
        int lelse = creer_label(), lend = creer_label();
        ecrire_condition(nd->children[0], lelse, 0, f);
        ecrire_instruction(nd->children[1], f);
        ecrire_indentation(f); fprintf(f, "goto L%d;\n", lend);
        ecrire_indentation(f); fprintf(f, "L%d:;\n", lelse);
        ecrire_instruction(nd->children[2], f);
        ecrire_indentation(f); fprintf(f, "L%d:;\n", lend);
        break;
    }

    case AST_WHILE: {  // boucle while
        if (nd->children_count < 2) break;
        int ltest = creer_label(), lloop = creer_label();
        ecrire_indentation(f); fprintf(f, "goto L%d;\n", ltest);
        ecrire_indentation(f); fprintf(f, "L%d:;\n", lloop);
        ecrire_instruction(nd->children[1], f);
        ecrire_indentation(f); fprintf(f, "L%d:;\n", ltest);
        ecrire_condition(nd->children[0], lloop, 1, f);
        break;
    }

    case AST_FOR: {  // boucle for
        // [init, test, update, corps]
        if (nd->children_count < 4) break;
        int ltest = creer_label(), lfor = creer_label();
        ecrire_instruction(nd->children[0], f);
        ecrire_indentation(f); fprintf(f, "goto L%d;\n", ltest);
        ecrire_indentation(f); fprintf(f, "L%d:;\n", lfor);
        ecrire_instruction(nd->children[3], f);
        {   char *v = ecrire_expression(nd->children[2], f); free(v); }
        ecrire_indentation(f); fprintf(f, "L%d:;\n", ltest);
        if (nd->children[1]->children_count > 0)
            ecrire_condition(nd->children[1]->children[0], lfor, 1, f);
        break;
    }

    case AST_RETURN: // return
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

// Ecrit une fonction en entier
static void ecrire_fonction(Ast_node *nd, FILE *f) {
    if (nd->children_count < 3) return;
    Ast_node *ts   = nd->children[0];
    Ast_node *decl = nd->children[1];
    Ast_node *body = nd->children[2];


    g_local = creer_symbole("__local__", 0, IDENTIFIER_SYMBOL);

    Ast_node *plist = obtenir_liste_params(decl);
    if (plist) extraire_arguments_fonction(plist, g_local);
    collecter_locales(body, g_local);

    // reset et init le corps dans buffer
    g_temp_compteur = 0;
    for (int i = 0; i < MAX_TEMPS; i++) { g_temp_type[i] = NULL; g_temp_sname[i] = NULL; }

    char *body_buf = NULL;
    size_t body_size = 0;
    FILE *body_f = open_memstream(&body_buf, &body_size);
    g_indentation = 1;
    for (int i = 0; i < body->children_count; i++)
        ecrire_instruction(body->children[i], body_f);
    fclose(body_f);

    fprintf(f, "%s %s(", chaine_type(ts, decl), nom_declarateur(decl));
    ecrire_parametres(plist, f);
    fprintf(f, ")\n{\n");

    // variables locales (excl. paramètres)
    for (int i = 0; i < g_local->child_count; i++) {
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

    // Déclarer les variables temp
    for (int i = 0; i < g_temp_compteur && i < MAX_TEMPS; i++) {
        const char *tt = g_temp_type[i] ? g_temp_type[i] : "void *";
        fprintf(f, "\t%s _t%d;\n", tt, i + 1);
    }

    if (body_buf) { fputs(body_buf, f); free(body_buf); }
    fprintf(f, "}\n");

    g_local = NULL;
}

// Ecrit les extern pour fonction ou variable
static void ecrire_extern(Ast_node *nd, FILE *f) {
    if (nd->children_count < 2) return;
    Ast_node *ts   = nd->children[0];
    Ast_node *decl = nd->children[1];
    Ast_node *plist = obtenir_liste_params(decl);
    const char *rt  = chaine_type(ts, decl);
    if (plist || trouver_decl_fonction(decl)) {
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

void write_code(Ast_node *prog, FILE *f) {
    if (!prog) return;
    g_label_compteur = 0;
    g_indentation = 0;
    g_local  = NULL;

    analyser_programme(prog);

    // pour chaque noeud de haut niveau
    for (int i = 0; i < prog->children_count; i++) {
        Ast_node *nd = prog->children[i];
        switch (nd->type) {
        case AST_STRUCT_DEFINITION: break;  // déjà fait
        case AST_EXTERN_DECLARATION: ecrire_extern(nd, f); break; // externes
        case AST_DECLARATION:        ecrire_decl_globale(nd, f); break;  // globales
        case AST_FUNCTION_DEFINITION:
            fprintf(f, "\n");
            ecrire_fonction(nd, f);
            break;
        default: break;
        }
    }
}

void print_error(Symbol *s, char *id, int line) {
    fprintf(stderr, "\033[1;31mErreur : '%s' déjà déclaré (ligne %d)\033[0m\n", id, line);
    (void)s;
}

void print_warning(Symbol *s, char *id, int line) {
    fprintf(stderr, "\033[1;35mAvertissement : redéfinition de '%s' (ligne %d)\033[0m\n", id, line);
    (void)s;
}

void print_color(char *couleur, char *texte) {
    printf("\033[1;%sm%s\033[0m", couleur, texte);
}

