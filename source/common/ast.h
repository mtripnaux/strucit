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
    int size;                     
    struct _Ast_node *parent;     
    struct _Ast_node **children;  
    int children_count;           
} Ast_node;



Ast_node *ast_create_node(Ast_type type);

Ast_node *create_node(Ast_type type, Ast_node *enfant1, Ast_node *enfant2);

Ast_node *create_int_leaf(int valeur);

Ast_node *create_id_leaf(char *nom);

void ast_add_child(Ast_node *parent, Ast_node *enfant);

void ast_free(Ast_node *noeud);

#endif
