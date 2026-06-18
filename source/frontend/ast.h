#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    AST_PROGRAM,                  // racine : liste de declarations/fonctions de tout le fichier
    AST_FUNCTION_DEFINITION,      // [type, declarateur, corps]
    AST_DECLARATION,              // declaration de variable : [type, declarateur]
    AST_EXTERN_DECLARATION,       // declaration "extern ..." : [type, declarateur]
    AST_STRUCT_DEFINITION,        // "struct Nom { champs }" : [nom, liste de champs]
    AST_STRUCT_FIELD_LIST,        // liste d'AST_STRUCT_FIELD
    AST_STRUCT_FIELD,             // un champ de struct : [type, declarateur]
    AST_PARAM_LIST,                // liste d'AST_PARAM (parametres de fonction)
    AST_PARAM,                    // un parametre : [type, declarateur]
    AST_TYPE_SPECIFIER,           // type de base : id = "int" ou "void"
    AST_STRUCT,                   // reference a un type struct existant : [nom]
    AST_STAR_DECLARATOR,          // declarateur prefixe d'une etoile : [declarateur interne]
    AST_DIRECT_DECLARATOR,        // declarateur de fonction sans params : [declarateur interne]
    AST_FUNC_DECLARATOR,          // declarateur de fonction avec params : [declarateur, AST_PARAM_LIST]
    AST_COMPOUND_STATEMENT,       // bloc { ... } : enfants = declarations puis instructions
    AST_STATEMENT_LIST,           // liste d'instructions (a plat)
    AST_EXPRESSION_STATEMENT,     // instruction reduite a une expression suivie de ';'
    AST_IF,                       // if sans else : [condition, corps]
    AST_IF_ELSE,                  // if/else : [condition, corps_if, corps_else]
    AST_WHILE,                    // while : [condition, corps]
    AST_FOR,                      // for : [init, test, increment, corps]
    AST_RETURN,                   // return [valeur] (0 ou 1 enfant)
    AST_ASSIGNMENT,                // affectation "a = b" : [cible, valeur]
    AST_OP,                       // operateur binaire arithmetique (+, -, *, /) : [gauche, droite], id = l'operateur
    AST_BOOL_OP,                  // comparaison (<, >, <=, >=, ==, !=) : [gauche, droite], id = l'operateur
    AST_BOOL_LOGIC,               // && ou || : [gauche, droite], id = l'operateur
    AST_POSTFIX,                  // appel de fonction "f(...)" : [fonction, AST_ARGUMENT_EXPRESSION_LIST?]
    AST_POSTFIX_POINTER,          // acces a un champ "p->champ" ou "p.champ" : [pointeur, nom du champ]
    AST_ARGUMENT_EXPRESSION_LIST, // liste d'expressions passees a un appel
    AST_UNARY,                    // operateur unaire (-, &, *) : [operateur, operande]
    AST_UNARY_SIZEOF,             // sizeof(...) : [argument]
    AST_IDENTIFIER,                // feuille : un nom (variable, fonction, champ, operateur...)
    AST_CONSTANT                  // feuille : une constante entiere, valeur dans ->value
} Ast_type;

/* Un noeud de l'AST. Tous les noeuds partagent la meme structure ; ce que
   signifient ->id, ->value et les enfants depend du ->type */
typedef struct _Ast_node {
    Ast_type type;
    char *id; // nom (identifiant, operateur, type de base...) ou NULL
    int value; // int pour AST_CONSTANT, sinon NULL
    int line; // numero de ligne source (via yyllineno)
    struct _Ast_node **children; // tableau dynamique des enfants
    int children_count;
} Ast_node;

/* Alloue un noeud vide du type donné (id=NULL, value=0, aucun enfant). */
Ast_node *ast_create_node(Ast_type type);

/* Alloue une feuille AST_CONSTANT portant la valeur entiere donnee. */
Ast_node *create_constant_leaf(int valeur);

/* Alloue une feuille AST_IDENTIFIER portant une copie de la chaine donnee. */
Ast_node *create_identifier_leaf(char *nom);

/* Ajoute enfant a la fin de la liste des enfants de parent (no-op si l'un
   des deux est NULL). */
void ast_add_child(Ast_node *parent, Ast_node *enfant);

/* Libere recursivement un noeud et tous ses enfants (et leur ->id). */
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

/* Noeud AST_FUNC_DECLARATOR ou AST_DIRECT_DECLARATOR d'un declarateur (ex:
   pour "int *(*f)(int n)", retrouve le noeud qui porte les parametres),
   ou NULL si ce declarateur n'est pas celui d'une fonction. */
Ast_node *ast_decl_fonction(Ast_node *decl);

/* Liste de parametres (AST_PARAM_LIST) d'un declarateur de fonction, ou
   NULL si la fonction n'a pas de parametres ou si ce n'est pas une
   fonction. */
Ast_node *ast_liste_parametres(Ast_node *decl);

#endif
