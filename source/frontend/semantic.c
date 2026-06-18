#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "semantic.h"
#include "symtable.h"
#include "erreurs.h"

/*
 * Plan de ce fichier (dans l'ordre du code, pas force l'ordre d'execution) :
 *   - petits utilitaires (nom_type, erreur/avertissement)
 *   - enregistrer_* : construisent table_globale en parcourant l'AST une
 *     fois (declaration de variable, de fonction, de struct)
 *   - type_expr + verifier_type_* : le "mini systeme de types" qui
 *     implemente les regles de l'enonce 3.1 sur les operateurs
 *   - verifier_appel / verifier_expression : verifient une expression
 *     (arite des appels, identifiants connus, types des operateurs)
 *   - verifier_noeud : verifie une instruction, et appelle les fonctions
 *     ci-dessus pour les expressions qu'elle contient
 *   - sem_analyse/sem_liberer : point d'entree, appele depuis main()
 *     (strucitfe.y)
 *
 * Le tout ne fait qu'UN SEUL parcours de l'AST (verifier_noeud descend
 * recursivement dans tout le programme). Pendant ce parcours, sem_local
 * existe uniquement le temps d'analyser le corps de LA fonction en cours :
 * il est cree au debut de AST_FUNCTION_DEFINITION et transfere dans
 * Symbol->locales a la fin (voir ce cas plus bas), pour etre reutilise par
 * codegen.c plus tard sans jamais etre reconstruit.
 */

/* Table des symboles locale a la fonction en cours d'analyse */
static Symbol *sem_local = NULL;
int sem_errors = 0;

static void sem_init(void) {
    symtable_creer();
    sem_errors = 0;
}

/* ->type_name d'un Symbol est une simple chaine : "int", "void", ou
   "struct" (jamais le nom de la structure elle-meme, qui va dans
   ->struct_name a part). Voir Symbol dans symbol.h. */
static const char *nom_type(Ast_node *n)
{
    if (!n) return "?";
    if (n->type == AST_TYPE_SPECIFIER) return n->id;
    if (n->type == AST_STRUCT || n->type == AST_STRUCT_DEFINITION)
        return "struct";
    return "?";
}

/* ── Erreurs*/

/* Toutes les erreurs semantiques passent par ici : delegue le formatage et
   l'affichage a erreurs.c (meme format que les erreurs lexicales/
   syntaxiques/systeme), en lui passant &sem_errors a incrementer (lu par
   main() dans strucitfe.y pour decider d'arreter la compilation avant
   toute generation de code). */
static void erreur(int ligne, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    erreur_semantique_v(&sem_errors, ligne, fmt, ap);
    va_end(ap);
}

/* Comme erreur(), mais n'incremente pas sem_errors : un avertissement
   n'empeche jamais la compilation de continuer. */
static void avertissement(int ligne, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    avertissement_semantique_v(ligne, fmt, ap);
    va_end(ap);
}

/* Une structure n'est pas manipulee par pointeur : erreur (cf enonce 3.1,
   "les structures ne peuvent etre manipulees que par le biais de pointeurs,
   [...] cette contrainte [...] est imposee par la semantique du langage").
   Appelee a chaque endroit ou un type "struct X" peut apparaitre :
   variable, champ de structure, parametre, type de retour. */
static void verifier_struct_par_pointeur(Ast_node *type_nd, Ast_node *decl_nd) {
    if (!type_nd || !decl_nd) return;
    if (type_nd->type != AST_STRUCT && type_nd->type != AST_STRUCT_DEFINITION) return;
    if (ast_est_pointeur(decl_nd)) return;

    Ast_node *id_nd = ast_premier_identifiant(decl_nd);
    erreur(id_nd ? id_nd->line : 0,
           "Une structure ne peut etre manipulée que par pointeur (\"%s\")",
           id_nd ? id_nd->id : "?");
}

/* Enregistrement des symboles*/

/* Enregistre une declaration de variable dans la table courante (locale
   si on est dans le corps d'une fonction, globale sinon — voir le choix
   de 'table' ci-dessous). Appelee pour CHAQUE declaration rencontree par
   verifier_noeud, qu'elle soit globale ou locale a une fonction. */
static void enregistrer_declaration(Ast_node *decl) {
    if (!decl || decl->children_count < 2) return;

    Ast_node *type_nd = decl->children[0];
    Ast_node *decl_nd = decl->children[1];
    Ast_node *id_nd   = ast_premier_identifiant(decl_nd);

    if (!id_nd) return;

    verifier_struct_par_pointeur(type_nd, decl_nd);

    char *nom = id_nd->id;
    Symbol *table = sem_local ? sem_local : table_globale;

    /* Redefinition */
    if (chercher_symbole_enfant(table, nom)) {
        avertissement(0, "Overriding identifier \"%s\"", nom);
    }

    int taille = 4;
    if (type_nd->type == AST_TYPE_SPECIFIER && strcmp(type_nd->id, "void") == 0)
        taille = 0;

    Symbol *s = creer_symbole(nom, taille, IDENTIFIER_SYMBOL);
    s->type_name = strdup(nom_type(type_nd));

    if (type_nd->type == AST_STRUCT || type_nd->type == AST_STRUCT_DEFINITION) {
        Ast_node *nom_struct = ast_premier_identifiant(type_nd);
        if (nom_struct) s->struct_name = strdup(nom_struct->id);
    }

    if (ast_est_pointeur(decl_nd)) s->pointer = true;

    ajouter_symbole_enfant(table, s);
}

/* Enregistre une definition de fonction (ou une declaration extern, via
   enregistrer_extern ci-dessous) dans table_globale : un FUNCTION_SYMBOL
   dont les ->children sont ses parametres, plus un symbole sentinelle
   "return" qui porte le type de retour (voir verifier_return : c'est plus
   simple que d'ajouter un champ dedie a Symbol juste pour ca). fs_out
   permet a l'appelant (le cas AST_FUNCTION_DEFINITION de verifier_noeud)
   de recuperer ce symbole pour analyser le corps juste apres. */
static void enregistrer_fonction(Ast_node *type_nd, Ast_node *decl_nd, Symbol **fs_out)
{
    Ast_node *nom_nd = ast_premier_identifiant(decl_nd);
    if (!nom_nd) return;

    verifier_struct_par_pointeur(type_nd, decl_nd);

    char *nom = nom_nd->id;
    Symbol *fs = creer_symbole(nom, 0, FUNCTION_SYMBOL);
    fs->type_name = strdup(nom_type(type_nd));
    if (type_nd->type == AST_STRUCT || type_nd->type == AST_STRUCT_DEFINITION) {
        Ast_node *ns = ast_premier_identifiant(type_nd);
        if (ns) fs->struct_name = strdup(ns->id);
    }
    if (ast_est_pointeur(decl_nd)) fs->pointer = true;

    Ast_node *plist = ast_liste_parametres(decl_nd);

    // Chaque element de plist vient de la regle "parameter_declaration :
    // declaration_specifiers declarator" : c'est toujours un AST_PARAM a
    // exactement 2 enfants (type, declarateur), quelle que soit la
    // complexite du declarateur (ast_nom_declarateur/ast_est_pointeur
    // savent deja suivre un declarateur de pointeur de fonction).
    if (plist) {
        for (int i = 0; i < plist->children_count; i++) {
            Ast_node *param = plist->children[i];
            if (param->children_count < 2) continue;
            Ast_node *ptype = param->children[0];
            Ast_node *pdecl = param->children[1];
            Ast_node *pid   = ast_nom_declarateur(pdecl);
            if (!pid) continue;
            verifier_struct_par_pointeur(ptype, pdecl);
            Symbol *ps = creer_symbole(pid->id, 4, IDENTIFIER_SYMBOL);
            ps->type_name = strdup(nom_type(ptype));
            ps->pointer = ast_est_pointeur(pdecl);
            ajouter_symbole_enfant(fs, ps);
        }
    }

    /* Symbole de retour : sentinelle "return" qui porte uniquement le
       type de retour de la fonction (jamais utilisee comme une vraie
       variable). chercher_symbole_enfant(fs, "return") la retrouve. */
    Symbol *ret = creer_symbole("return", 0, IDENTIFIER_SYMBOL);
    ret->type_name = strdup(fs->type_name);
    if (fs->struct_name) ret->struct_name = strdup(fs->struct_name);
    ret->pointer = fs->pointer;
    ajouter_symbole_enfant(fs, ret);

    ajouter_symbole_enfant(table_globale, fs);
    if (fs_out) *fs_out = fs;
}

/* Enregistre une declaration extern (fonction ou variable externe) :
   memes regles qu'une fonction normale, juste sans corps a analyser. */
static void enregistrer_extern(Ast_node *decl)
{
    if (!decl || decl->children_count < 2) return;
    enregistrer_fonction(decl->children[0], decl->children[1], NULL);
}

/* Enregistre une definition de struct dans la table globale : un
   STRUCT_SYMBOL dont les ->children sont ses champs, chacun avec son
   ->offset (position en octets) deja calcule ici. C'est cet ->offset que
   codegen.c utilise directement pour traduire "p->champ" en "p + offset"
   (voir obtenir_offset_champ dans codegen.c) : si ce calcul est faux ou
   absent, TOUT acces a un champ de structure produit du code faux, sans
   que rien ne le signale (bug reel rencontre et corrige sur ce projet). */
static void enregistrer_struct(Ast_node *def)
{
    if (!def) return;

    Ast_node *nom_nd = ast_premier_identifiant(def);
    if (!nom_nd) return;

    Symbol *ss = creer_symbole(nom_nd->id, 0, STRUCT_SYMBOL);
    ss->type_name = strdup("struct");

    /* Champs : chaque champ occupe 4 octets, a la suite des precedents
       (necessaire pour l'arithmetique de pointeur generee par le backend,
       ex: p->suivant -> p + offset) */
    int off = 0;
    for (int i = 0; i < def->children_count; i++) {
        Ast_node *child = def->children[i];
        if (child->type == AST_STRUCT_FIELD_LIST) {
            for (int j = 0; j < child->children_count; j++) {
                Ast_node *field = child->children[j];
                if (field->type == AST_STRUCT_FIELD && field->children_count >= 2) {
                    Ast_node *ftype = field->children[0];
                    Ast_node *fdecl = field->children[1];
                    Ast_node *fid   = ast_premier_identifiant(fdecl);
                    if (!fid) continue;
                    verifier_struct_par_pointeur(ftype, fdecl);
                    Symbol *fs = creer_symbole(fid->id, 4, IDENTIFIER_SYMBOL);
                    fs->type_name = strdup(nom_type(ftype));
                    if (ast_est_pointeur(fdecl)) fs->pointer = true;
                    if (ftype->type == AST_STRUCT) {
                        // Champ "struct Y *suivant" : on retient AUSSI le
                        // nom Y, pour permettre un acces chaine
                        // (a->suivant->suivant->champ) plus loin dans le
                        // code (cf g_expr_sname dans codegen.c).
                        fs->pointer = true;
                        Ast_node *sn = ast_premier_identifiant(ftype);
                        if (sn) fs->struct_name = strdup(sn->id);
                    }
                    fs->offset = off;
                    off += 4;
                    ajouter_symbole_enfant(ss, fs);
                }
            }
        }
    }
    ss->size = off;

    ajouter_symbole_enfant(table_globale, ss);
}

/*Verification des expressions */

static void verifier_expression(Ast_node *n, int ligne);

/* Type minimal d'une expression, juste assez pour les regles de l'enonce :
   "Toutes les operations binaires sont autorisees sur des int. Seules les
   operations binaires suivantes sont autorisees sur des pointeurs [...]
   Pour les operations unaires, * et ->champ ne peuvent s'appliquer qu'a
   une variable de type pointeur, & ne peut s'appliquer qu'a une variable
   de type int ou a une fonction, - ne peut s'appliquer qu'a une variable
   de type int." Quand le type ne peut pas etre determine simplement (appel
   de fonction, champ de structure, ...), on renvoie "inconnu" et on ne
   signale rien : on reste minimal et on evite les faux positifs. */
typedef enum { T_INCONNU = -1, T_INT = 0, T_POINTEUR = 1, T_FONCTION = 2 } Type_expr;

/* Determine le type (au sens ci-dessus) d'une expression, en se limitant
   aux quelques formes pour lesquelles c'est immediat : constante (=int),
   identifiant (on regarde le Symbol correspondant), ou unaire (-, &, *
   ont chacun un type de resultat fixe ou facilement deductible). Pour
   tout le reste (appel de fonction, champ de structure, operation
   binaire...) : T_INCONNU, deliberement, pour ne jamais se tromper. */
static Type_expr type_expr(Ast_node *n)
{
    if (!n) return T_INCONNU;

    switch (n->type) {
    case AST_CONSTANT:
        return T_INT;

    case AST_IDENTIFIER: {
        Symbol *s = sem_local ? chercher_symbole_enfant(sem_local, n->id) : NULL;
        if (!s) s = table_globale ? chercher_symbole_enfant(table_globale, n->id) : NULL;
        if (!s) return T_INCONNU;
        if (s->type == FUNCTION_SYMBOL) return T_FONCTION;
        return s->pointer ? T_POINTEUR : T_INT;
    }

    case AST_UNARY:
        if (n->children_count < 2 || !n->children[0]->id) return T_INCONNU;
        if (strcmp(n->children[0]->id, "&") == 0) return T_POINTEUR;
        if (strcmp(n->children[0]->id, "-") == 0) return T_INT;
        if (strcmp(n->children[0]->id, "*") == 0)
            return type_expr(n->children[1]) == T_POINTEUR ? T_INT : T_INCONNU;
        return T_INCONNU;

    default:
        return T_INCONNU;
    }
}

/* Si ligne == 0 (cas habituel des appels depuis verifier_noeud), on essaie
   de retrouver une ligne utile via le premier identifiant du sous-arbre. */
static int ligne_effective(Ast_node *n, int ligne)
{
    if (ligne > 0) return ligne;
    Ast_node *id_nd = ast_premier_identifiant(n);
    return (id_nd && id_nd->line > 0) ? id_nd->line : ligne;
}

/* * et ->champ ne s'appliquent qu'a un pointeur ; & qu'a un int ou une
   fonction ; - qu'a un int. */
static void verifier_type_unaire(Ast_node *n, int ligne)
{
    if (n->children_count < 2 || !n->children[0]->id) return;
    char *op = n->children[0]->id;
    Type_expr t = type_expr(n->children[1]);
    if (t == T_INCONNU) return;
    ligne = ligne_effective(n->children[1], ligne);

    if (strcmp(op, "-") == 0 && t != T_INT)
        erreur(ligne, "L'operateur unaire '-' ne peut s'appliquer qu'a une variable de type int");
    else if (strcmp(op, "*") == 0 && t != T_POINTEUR)
        erreur(ligne, "L'operateur unaire '*' ne peut s'appliquer qu'a une variable de type pointeur");
    else if (strcmp(op, "&") == 0 && t == T_POINTEUR)
        erreur(ligne, "L'operateur '&' ne peut s'appliquer qu'a une variable de type int ou a une fonction");
}

/* x->champ : seul x doit etre un pointeur (le nom du champ n'est pas une
   expression, donc rien a verifier de ce cote). */
static void verifier_type_fleche(Ast_node *n, int ligne)
{
    if (n->children_count < 1) return;
    Type_expr t = type_expr(n->children[0]);
    if (t != T_INCONNU && t != T_POINTEUR)
        erreur(ligne_effective(n->children[0], ligne), "L'operateur '->' ne peut s'appliquer qu'a une variable de type pointeur");
}

/* + - * / : seules addition/soustraction d'un entier a un pointeur, et
   soustraction entre deux pointeurs, sont autorisees sur des pointeurs. */
static void verifier_type_binaire(Ast_node *n, int ligne)
{
    if (n->children_count < 2) return;
    Type_expr tl = type_expr(n->children[0]);
    Type_expr tr = type_expr(n->children[1]);
    if (tl == T_INCONNU || tr == T_INCONNU) return;

    int l_ptr = (tl == T_POINTEUR), r_ptr = (tr == T_POINTEUR);
    if (!l_ptr && !r_ptr) return;  /* int op int : toujours autorise */
    ligne = ligne_effective(n, ligne);

    if (strcmp(n->id, "+") == 0) {
        if (l_ptr && r_ptr)
            erreur(ligne, "Addition interdite entre deux pointeurs");
    } else if (strcmp(n->id, "-") == 0) {
        if (!l_ptr && r_ptr)
            erreur(ligne, "Soustraction d'un pointeur a un entier interdite");
    } else {
        erreur(ligne, "Les operateurs '*' et '/' ne sont pas autorises sur des pointeurs");
    }
}

/* Verifie un appel de fonction "f(...)" ou "(*f)(...)" : la verification
   se fait en trois cas distincts, du plus permissif au plus strict. */
static void verifier_appel(Ast_node *postfix, int ligne)
{
    if (!postfix || postfix->children_count < 1) return;

    /* Recupere le numero de ligne depuis le noeud si disponible */
    int line = postfix->line > 0 ? postfix->line : ligne;

    /* Cas 1 : appel via pointeur de fonction, ex "(*fact)(n-1)". On ne
       connait pas statiquement quelle fonction sera realement appelee
       (c'est une variable), donc impossible de verifier son arite ici :
       on se contente de verifier les arguments eux-memes. */
    if (postfix->children[0]->type == AST_UNARY) {
        if (postfix->children_count >= 2)
            verifier_expression(postfix->children[1], line);
        return;
    }

    Ast_node *fn_nd = ast_premier_identifiant(postfix->children[0]);
    if (!fn_nd) return;
    char *nom = fn_nd->id;
    /* Utilise la ligne de l identifiant si disponible */
    if (fn_nd->line > 0) line = fn_nd->line;

    /* Cas 2 : nom connu dans la table globale -> c'est une vraie fonction,
       on peut verifier le nombre d'arguments. */
    Symbol *fs = table_globale ? chercher_symbole_enfant(table_globale, (char *)nom) : NULL;
    if (fs) {
        int nb_args = 0;
        if (postfix->children_count >= 2 &&
            postfix->children[1]->type == AST_ARGUMENT_EXPRESSION_LIST)
            nb_args = postfix->children[1]->children_count;
        int nb_params = 0;
        for (int i = 0; i < fs->child_count; i++) {
            if (fs->children[i]->id && strcmp(fs->children[i]->id, "return") != 0)
                nb_params++;
        }
        if (nb_params > 0 && nb_args != nb_params && fs->type == FUNCTION_SYMBOL)
            erreur(line, "Function \"%s\" requires %d arguments but %d were given",
                   nom, nb_params, nb_args);
        if (postfix->children_count >= 2)
            verifier_expression(postfix->children[1], line);
        return;
    }

    /* Cas 3 : pas une fonction globale, mais un parametre local connu
       -> c'est un pointeur de fonction passe en parametre (ex: la
       fonction "applique" dans edge_funcptr.c), donc legitime : OK sans
       verifier l'arite (meme raison que le cas 1). */
    if (sem_local && chercher_symbole_enfant(sem_local, (char *)nom)) {
        if (postfix->children_count >= 2)
            verifier_expression(postfix->children[1], line);
        return;
    }

    /* Aucun des trois cas : identifiant vraiment inconnu. */
    erreur(line, "Identifiant inconnuuuu \"%s\"", nom);
}

/* Verifie une expression : descend recursivement dedans en appliquant a
   chaque noeud la regle qui le concerne (type des operandes pour les
   operateurs, arite pour les appels). Ne verifie PAS qu'un identifiant
   simple existe (cf le commentaire dans le cas AST_IDENTIFIER) : seuls
   les appels de fonction sont verifies pour existence, le reste du temps
   on prefere ne rien signaler que produire un faux positif. */
static void verifier_expression(Ast_node *n, int ligne)
{
    if (!n) return;

    switch (n->type) {
    case AST_IDENTIFIER: {
        /* Ignore les operateurs et les builtins connus */
        if (!n->id) break;
        if (strcmp(n->id, "-")    == 0 || strcmp(n->id, "*")    == 0 ||
            strcmp(n->id, "&")    == 0 || strcmp(n->id, "++")   == 0 ||
            strcmp(n->id, "--")   == 0 || strcmp(n->id, "int")  == 0 ||
            strcmp(n->id, "void") == 0 || strcmp(n->id, "sizeof") == 0) break;
        /* On ne verifie pas les identifiants simples pour eviter les faux positifs
           sur les parametres de pointeurs de fonction et les variables de struct */
        break;
    }
    case AST_POSTFIX_POINTER:
        /* x->champ : on verifie seulement x (pas le nom du champ) */
        verifier_type_fleche(n, ligne);
        if (n->children_count >= 1)
            verifier_expression(n->children[0], ligne);
        break;
    case AST_UNARY:
        verifier_type_unaire(n, ligne);
        if (n->children_count >= 2)
            verifier_expression(n->children[1], ligne);
        else if (n->children_count == 1)
            verifier_expression(n->children[0], ligne);
        break;
    case AST_OP:
        verifier_type_binaire(n, ligne);
        for (int i = 0; i < n->children_count; i++)
            verifier_expression(n->children[i], ligne);
        break;
    case AST_POSTFIX:
        if (n->children_count >= 1) {
            Ast_node *second = (n->children_count >= 2) ? n->children[1] : NULL;
            if (second && second->type == AST_ARGUMENT_EXPRESSION_LIST)
                verifier_appel(n, ligne);
            else if (second == NULL)
                verifier_appel(n, ligne);
            else
                verifier_expression(n->children[0], ligne);
        }
        break;
    case AST_ASSIGNMENT:
        verifier_expression(n->children[0], ligne);
        verifier_expression(n->children[1], ligne);
        break;
    default:
        // Comparaisons (AST_BOOL_OP), && / || (AST_BOOL_LOGIC), et tout
        // ce qui n'a pas de cas dedie : pas de regle de typage specifique
        // a appliquer (l'enonce dit explicitement que les conditions
        // comparent "deux expressions quelconques"), on se contente de
        // continuer la descente dans les enfants.
        for (int i = 0; i < n->children_count; i++)
            verifier_expression(n->children[i], ligne);
        break;
    }
}

/*Verification des instructions*/

static void verifier_noeud(Ast_node *n);

/* Verifie qu'un "return" est coherent avec le type de retour de la
   fonction fn (cf le symbole sentinelle "return" cree dans
   enregistrer_fonction) : presence/absence d'une valeur, et type de
   cette valeur si presente. */
static void verifier_return(Ast_node *n, Symbol *fn)
{
    if (!fn) return;

    Symbol *ret = chercher_symbole_enfant(fn, "return");
    if (!ret) return;

    bool attend_valeur = ret->type_name && strcmp(ret->type_name, "void") != 0;
    bool a_valeur      = (n->children_count > 0);

    if (attend_valeur && !a_valeur)
        erreur(0, "Function \"%s\" must return a value", fn->id);
    else if (!attend_valeur && a_valeur)
        erreur(0, "Function \"%s\" must not return a value", fn->id);

    if (a_valeur)
        verifier_expression(n->children[0], 0);
}

/* Verifie une instruction (au sens large : declaration, expression,
   structure de controle...) en descendant recursivement dans tout l'AST.
   C'est le point d'entree unique du parcours : sem_analyse() l'appelle
   sur la racine, et chaque cas rappelle verifier_noeud sur ses propres
   enfants. */
static void verifier_noeud(Ast_node *n)
{
    if (!n) return;

    switch (n->type) {

    case AST_PROGRAM:
        for (int i = 0; i < n->children_count; i++)
            verifier_noeud(n->children[i]);
        break;

    case AST_EXTERN_DECLARATION:
        enregistrer_extern(n);
        break;

    case AST_STRUCT_DEFINITION:
        enregistrer_struct(n);
        break;

    case AST_DECLARATION:
        enregistrer_declaration(n);
        break;

    case AST_FUNCTION_DEFINITION: {
        if (n->children_count < 3) break;

        Ast_node *type_nd = n->children[0];
        Ast_node *decl_nd = n->children[1];
        Ast_node *body_nd = n->children[2];

        Symbol *fs = NULL;
        enregistrer_fonction(type_nd, decl_nd, &fs);
        if (!fs) break;

        /* Table locale pour TOUTE la duree de l'analyse de cette
           fonction : sem_local est lu par type_expr/verifier_appel pour
           toute expression rencontree dans le corps. */
        sem_local = creer_symbole("__local__", 0, FUNCTION_SYMBOL);

        /* Ajoute les parametres a la table locale (copies depuis fs, pas
           les memes objets : fs->children appartient au FUNCTION_SYMBOL
           global, sem_local est une table a part qui finira dans
           fs->locales juste en dessous). */
        for (int i = 0; i < fs->child_count; i++) {
            if (strcmp(fs->children[i]->id, "return") != 0) {
                Symbol *copy = creer_symbole(fs->children[i]->id,
                                             fs->children[i]->size,
                                             IDENTIFIER_SYMBOL);
                copy->type_name   = fs->children[i]->type_name ?
                                    strdup(fs->children[i]->type_name) : NULL;
                copy->struct_name = fs->children[i]->struct_name ?
                                    strdup(fs->children[i]->struct_name) : NULL;
                copy->pointer     = fs->children[i]->pointer;
                ajouter_symbole_enfant(sem_local, copy);
            }
        }

        verifier_noeud(body_nd);

        /* La table locale (parametres + declarations) est transferee a la
           fonction : le generateur de code la reutilise telle quelle au
           lieu de la reconstruire depuis l'AST. */
        fs->locales = sem_local;
        sem_local = NULL;
        break;
    }

    case AST_COMPOUND_STATEMENT:
        for (int i = 0; i < n->children_count; i++)
            verifier_noeud(n->children[i]);
        break;

    case AST_STATEMENT_LIST:
        for (int i = 0; i < n->children_count; i++)
            verifier_noeud(n->children[i]);
        break;

    case AST_EXPRESSION_STATEMENT:
        if (n->children_count > 0)
            verifier_expression(n->children[0], 0);
        break;

    case AST_RETURN: {
        // "La fonction courante" = le dernier FUNCTION_SYMBOL ajoute a
        // table_globale. Ca marche car ce projet ne supporte pas les
        // fonctions imbriquees et verifier_noeud descend dans le corps
        // d'une fonction immediatement apres l'avoir enregistree (cf le
        // cas AST_FUNCTION_DEFINITION ci-dessus) : aucune autre fonction
        // n'a pu etre ajoutee entre-temps.
        Symbol *fn = NULL;
        if (table_globale)
            fn = table_globale->child_count > 0 ?
                 table_globale->children[table_globale->child_count - 1] : NULL;
        verifier_return(n, fn);
        break;
    }

    case AST_IF:
        verifier_expression(n->children[0], 0);
        verifier_noeud(n->children[1]);
        break;

    case AST_IF_ELSE:
        verifier_expression(n->children[0], 0);
        verifier_noeud(n->children[1]);
        verifier_noeud(n->children[2]);
        break;

    case AST_WHILE:
        verifier_expression(n->children[0], 0);
        verifier_noeud(n->children[1]);
        break;

    case AST_FOR:
        verifier_noeud(n->children[0]);       // init : une instruction (expression_statement)
        verifier_noeud(n->children[1]);       // test : une instruction (expression_statement)
        verifier_expression(n->children[2], 0); // increment : une expression nue, pas une instruction
        verifier_noeud(n->children[3]);       // corps
        break;

    default:
        for (int i = 0; i < n->children_count; i++)
            verifier_noeud(n->children[i]);
        break;
    }
}

/*Point d'entrée public*/

void sem_analyse(Ast_node *programme)
{
    sem_init();
    verifier_noeud(programme);
}

void sem_liberer(void)
{
    symtable_liberer();
}
