#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "semantic.h"

/*Tables des symboles globale / locale */

Symbol *sem_global = NULL;
Symbol *sem_local  = NULL;
int     sem_errors = 0;

void sem_init(void)
{
    sem_global = creer_symbole("__global__", 0, FUNCTION_SYMBOL);
    sem_errors = 0;
}


static Symbol *chercher(const char *nom)
{
    Symbol *s = sem_local  ? chercher_symbole_enfant(sem_local,  (char *)nom) : NULL;
    if (!s) s = sem_global ? chercher_symbole_enfant(sem_global, (char *)nom) : NULL;
    return s;
}

static const char *nom_type(Ast_node *n)
{
    if (!n) return "?";
    if (n->type == AST_TYPE_SPECIFIER) return n->id;
    if (n->type == AST_STRUCT || n->type == AST_STRUCT_DEFINITION)
        return "struct";
    return "?";
}

/* Trouve le premier IDENTIFIER dans un sous-arbre */
static Ast_node *premier_id(Ast_node *n)
{
    if (!n) return NULL;
    if (n->type == AST_IDENTIFIER) return n;
    for (int i = 0; i < n->children_count; i++) {
        Ast_node *r = premier_id(n->children[i]);
        if (r) return r;
    }
    return NULL;
}

/* ── Erreurs*/

static void erreur(int ligne, const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "\033[1;31mError:\033[0m ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, " (line %d)\n", ligne);
    sem_errors++;
}

static void avertissement(int ligne, const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "\033[1;35mWarning:\033[0m ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, " (line %d)\n", ligne);
}

/* Enregistrement des symboles*/

/* Enregistre une declaration de variable dans la table courante */
static void enregistrer_declaration(Ast_node *decl)
{
    if (!decl || decl->children_count < 2) return;

    Ast_node *type_nd = decl->children[0];
    Ast_node *decl_nd = decl->children[1];
    Ast_node *id_nd   = premier_id(decl_nd);

    if (!id_nd) return;

    char *nom = id_nd->id;
    Symbol *table = sem_local ? sem_local : sem_global;

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
        Ast_node *nom_struct = premier_id(type_nd);
        if (nom_struct) s->struct_name = strdup(nom_struct->id);
    }

    if (decl_nd->type == AST_STAR_DECLARATOR) s->pointer = true;

    ajouter_symbole_enfant(table, s);
}

/* Enregistre une definition de fonction */
static void enregistrer_fonction(Ast_node *type_nd, Ast_node *decl_nd, Symbol **fs_out)
{
    Ast_node *nom_nd = premier_id(decl_nd);
    if (!nom_nd) return;

    char *nom = nom_nd->id;
    Symbol *fs = creer_symbole(nom, 0, FUNCTION_SYMBOL);
    fs->type_name = strdup(nom_type(type_nd));
    if (type_nd->type == AST_STRUCT || type_nd->type == AST_STRUCT_DEFINITION) {
        Ast_node *ns = premier_id(type_nd);
        if (ns) fs->struct_name = strdup(ns->id);
    }
    if (decl_nd->type == AST_STAR_DECLARATOR) fs->pointer = true;

    /* Cherche la liste de parametres recursivement */
    Ast_node *plist = NULL;
    for (int i = 0; i < decl_nd->children_count; i++) {
        if (decl_nd->children[i]->type == AST_PARAM_LIST) {
            plist = decl_nd->children[i];
            break;
        }
    }

    if (plist) {
        for (int i = 0; i < plist->children_count; i++) {
            Ast_node *param = plist->children[i];
            if (param->type != AST_PARAM || param->children_count < 2) continue;
            Ast_node *ptype = param->children[0];
            Ast_node *pdecl = param->children[1];
            Ast_node *pid   = premier_id(pdecl);
            if (!pid) continue;
            Symbol *ps = creer_symbole(pid->id, 4, IDENTIFIER_SYMBOL);
            ps->type_name = strdup(nom_type(ptype));
            if (pdecl->type == AST_STAR_DECLARATOR) ps->pointer = true;
            ajouter_symbole_enfant(fs, ps);
        }
    }

    /* Symbole de retour */
    Symbol *ret = creer_symbole("return", 0, IDENTIFIER_SYMBOL);
    ret->type_name = strdup(fs->type_name);
    if (fs->struct_name) ret->struct_name = strdup(fs->struct_name);
    ret->pointer = fs->pointer;
    ajouter_symbole_enfant(fs, ret);

    ajouter_symbole_enfant(sem_global, fs);
    if (fs_out) *fs_out = fs;
}

/* Enregistre une declaration extern */
static void enregistrer_extern(Ast_node *decl)
{
    if (!decl || decl->children_count < 2) return;
    enregistrer_fonction(decl->children[0], decl->children[1], NULL);
}

/* Enregistre une definition de struct dans la table globale */
static void enregistrer_struct(Ast_node *def)
{
    if (!def) return;

    Ast_node *nom_nd = premier_id(def);
    if (!nom_nd) return;

    Symbol *ss = creer_symbole(nom_nd->id, 0, STRUCT_SYMBOL);
    ss->type_name = strdup("struct");

    /* Champs */
    for (int i = 0; i < def->children_count; i++) {
        Ast_node *child = def->children[i];
        if (child->type == AST_STRUCT_FIELD_LIST) {
            for (int j = 0; j < child->children_count; j++) {
                Ast_node *field = child->children[j];
                if (field->type == AST_STRUCT_FIELD && field->children_count >= 2) {
                    Ast_node *ftype = field->children[0];
                    Ast_node *fdecl = field->children[1];
                    Ast_node *fid   = premier_id(fdecl);
                    if (!fid) continue;
                    Symbol *fs = creer_symbole(fid->id, 4, IDENTIFIER_SYMBOL);
                    fs->type_name = strdup(nom_type(ftype));
                    if (fdecl->type == AST_STAR_DECLARATOR) fs->pointer = true;
                    ajouter_symbole_enfant(ss, fs);
                }
            }
        }
    }

    ajouter_symbole_enfant(sem_global, ss);
}

/*Verification des expressions */

static void verifier_expression(Ast_node *n, int ligne);

static void verifier_appel(Ast_node *postfix, int ligne)
{
    if (!postfix || postfix->children_count < 1) return;

    Ast_node *fn_nd = premier_id(postfix->children[0]);
    if (!fn_nd) return;

    char *nom = fn_nd->id;
    Symbol *fs = chercher(nom);

    if (!fs) {
        erreur(ligne, "Unknown identifier \"%s\"", nom);
        return;
    }

    /* Compte les arguments fournis */
    int nb_args = 0;
    if (postfix->children_count >= 2 &&
        postfix->children[1]->type == AST_ARGUMENT_EXPRESSION_LIST)
        nb_args = postfix->children[1]->children_count;

    /* Compte les parametres attendus (on exclut "return") */
    int nb_params = 0;
    for (int i = 0; i < fs->child_count; i++) {
        if (fs->children[i]->id && strcmp(fs->children[i]->id, "return") != 0)
            nb_params++;
    }

    if (nb_args != nb_params && fs->type == FUNCTION_SYMBOL) {
        erreur(ligne, "Function \"%s\" requires %d arguments but %d were given",
               nom, nb_params, nb_args);
    }

    /* Verifie les arguments */
    if (postfix->children_count >= 2)
        verifier_expression(postfix->children[1], ligne);
}

static void verifier_expression(Ast_node *n, int ligne)
{
    if (!n) return;

    switch (n->type) {
    case AST_IDENTIFIER: {
        /* Ignore les feuilles qui sont des operateurs (-, *, &, ++, --) */
        if (!n->id) break;
        if (strcmp(n->id, "-")    == 0 || strcmp(n->id, "*")  == 0 ||
            strcmp(n->id, "&")    == 0 || strcmp(n->id, "++") == 0 ||
            strcmp(n->id, "--")   == 0 || strcmp(n->id, "int") == 0 ||
            strcmp(n->id, "void") == 0 || strcmp(n->id, "malloc") == 0 ||
            strcmp(n->id, "free") == 0 || strcmp(n->id, "NULL") == 0) break;
        Symbol *s = chercher(n->id);
        if (!s)
            erreur(ligne, "Unknown identifier \"%s\"", n->id);
        break;
    }
    case AST_POSTFIX_POINTER:
        /* x->champ : on verifie seulement x (pas le nom du champ) */
        if (n->children_count >= 1)
            verifier_expression(n->children[0], ligne);
        break;
    case AST_UNARY:
        /* Pointeur de fonction (*fact) : on skip l operateur, on verifie l operande */
        if (n->children_count >= 2)
            verifier_expression(n->children[1], ligne);
        else if (n->children_count == 1)
            verifier_expression(n->children[0], ligne);
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
        for (int i = 0; i < n->children_count; i++)
            verifier_expression(n->children[i], ligne);
        break;
    }
}

/*Verification des instructions*/

static void verifier_noeud(Ast_node *n);

static void verifier_return(Ast_node *n, Symbol *fn)
{
    if (!fn) return;

    /* Cherche le type de retour attendu */
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

        /* Analyse du corps */
        sem_local = creer_symbole("__local__", 0, FUNCTION_SYMBOL);

        /* Ajoute les parametres a la table locale */
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
        /* Trouve la fonction courante */
        Symbol *fn = NULL;
        if (sem_global)
            fn = sem_global->child_count > 0 ?
                 sem_global->children[sem_global->child_count - 1] : NULL;
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
        verifier_noeud(n->children[0]);
        verifier_noeud(n->children[1]);
        verifier_expression(n->children[2], 0);
        verifier_noeud(n->children[3]);
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
