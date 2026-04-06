#include "ast.h"

Ast_node *ast_create_empty_node(Ast_type type) {
    Ast_node *node = (Ast_node *)malloc(sizeof(Ast_node));
    node->type = type;
    node->children = NULL;
    node->children_count = 0;
    node->id = NULL;
    node->value = 0;
    return node;
}

Ast_node *create_node(Ast_type type, Ast_node *c1, Ast_node *c2) {
    Ast_node *node = ast_create_empty_node(type);
    if (c1) ast_add_child(node, c1);
    if (c2) ast_add_child(node, c2);
    return node;
}

Ast_node *create_int_leaf(int value) {
    Ast_node *node = ast_create_empty_node(AST_CONSTANT);
    node->value = value;
    return node;
}

Ast_node *create_id_leaf(char *name) {
    Ast_node *node = ast_create_empty_node(AST_IDENTIFIER);
    node->id = strdup(name);
    return node;
}

void ast_add_child(Ast_node *parent, Ast_node *child) {
    if (!parent || !child) return;
    parent->children_count++;
    parent->children = (Ast_node **)realloc(parent->children, sizeof(Ast_node *) * parent->children_count);
    parent->children[parent->children_count - 1] = child;
    child->parent = parent;
}