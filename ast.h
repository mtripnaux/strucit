#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum _Ast_type {
    AST_PROGRAM, AST_FUNC_DEF, AST_RETURN, AST_FOR, AST_WHILE, AST_IF, AST_IF_ELSE,
    AST_COMPOUND_STATEMENT, AST_EXPRESSION_STATEMENT, AST_DECLARATION,
    AST_ASSIGN, AST_ADD, AST_SUB, AST_MUL, AST_DIV,
    AST_LT, AST_GT, AST_LE, AST_GE, AST_EQ, AST_NE,
    AST_AND, AST_OR, AST_LSHIFT, AST_RSHIFT,
    AST_POST_INC, AST_POST_DEC, AST_PTR_OP, AST_DOT, AST_CALL,
    AST_IDENTIFIER, AST_CONSTANT, AST_UNARY, AST_SIZEOF
} Ast_type;

typedef struct _Ast_node {
    Ast_type type;
    char *id;
    int value;
    struct _Ast_node *parent;
    struct _Ast_node **children;
    int children_count;
    // ... tes autres champs (sethi_ullman, etc.)
} Ast_node;

// Prototypes synchronisés avec structfe.y
Ast_node *create_node(Ast_type type, Ast_node *child1, Ast_node *child2);
Ast_node *create_int_leaf(int value);
Ast_node *create_id_leaf(char *name);
void ast_add_child(Ast_node *parent, Ast_node *child);

#endif