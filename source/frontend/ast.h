#ifndef AST_H
#define AST_H
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION_DEFINITION,
    AST_DECLARATION,
    AST_EXTERN_DECLARATION,
    AST_STRUCT_DEFINITION,
    AST_STRUCT_FIELD_LIST,
    AST_STRUCT_FIELD,
    AST_PARAM_LIST,
    AST_PARAM,
    AST_TYPE_SPECIFIER,
    AST_STRUCT,
    AST_STAR_DECLARATOR,
    AST_DIRECT_DECLARATOR,
    AST_FUNC_DECLARATOR,
    AST_COMPOUND_STATEMENT,
    AST_STATEMENT_LIST,
    AST_EXPRESSION_STATEMENT,
    AST_IF,
    AST_IF_ELSE,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_ASSIGNMENT,
    AST_OP,
    AST_BOOL_OP,
    AST_BOOL_LOGIC,
    AST_POSTFIX,
    AST_POSTFIX_POINTER,
    AST_ARGUMENT_EXPRESSION_LIST,
    AST_UNARY,
    AST_UNARY_SIZEOF,
    AST_IDENTIFIER,
    AST_CONSTANT
} Ast_type;

typedef struct _Ast_node {
    Ast_type type;
    char *id;
    int value;
    int line;
    struct _Ast_node **children;
    int children_count;
} Ast_node;

Ast_node *ast_create_node(Ast_type type);

Ast_node *create_int_leaf(int valeur);

Ast_node *create_id_leaf(char *nom);

void ast_add_child(Ast_node *parent, Ast_node *enfant);

void ast_free(Ast_node *noeud);

/* Le declarateur a-t-il une etoile en tete (ex: struct liste *p) ? Ne suit
   que la chaine principale (children[0]) : pour un declarateur de fonction,
   children[1] est la liste de parametres et ne doit jamais etre inspectee. */
int ast_est_pointeur(Ast_node *decl);

/* Identifiant porte par un declarateur (ex: dans int (*f)(int n), retourne
   f). Ne suit que la chaine principale, jamais une liste de parametres. */
Ast_node *ast_nom_declarateur(Ast_node *decl);

/* Premier identifiant trouve n'importe ou dans le sous-arbre (recherche
   large, utile quand on ne sait pas a priori ou il se trouve). */
Ast_node *ast_premier_identifiant(Ast_node *n);

#endif
