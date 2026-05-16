#include "ast.h"
 
static Ast_node *alloc_node(Ast_type type) {
    Ast_node *n = malloc(sizeof(Ast_node));
    n->type = type;
    n->id = NULL;
    n->value = 0;
    n->size = 0;
    n->line = 0;
    n->parent = NULL;
    n->children = NULL;
    n->children_count = 0;
    return n;
}

Ast_node *ast_create_node(Ast_type type) {
    return alloc_node(type);
}

Ast_node *create_node(Ast_type type, Ast_node *c1, Ast_node *c2) {
    Ast_node *n = alloc_node(type);
    if (c1) ast_add_child(n, c1);
    if (c2) ast_add_child(n, c2);
    return n;
}

Ast_node *create_int_leaf(int value) {
    Ast_node *n = alloc_node(AST_CONSTANT);
    n->value = value;
    return n;
}

Ast_node *create_id_leaf(char *name) {
    Ast_node *n = alloc_node(AST_IDENTIFIER);
    n->id = strdup(name);
    return n;
}

void ast_add_child(Ast_node *parent, Ast_node *child) {
    if (!parent || !child) return;
    parent->children_count++;
    parent->children = realloc(parent->children,
                               sizeof(Ast_node *) * parent->children_count);
    parent->children[parent->children_count - 1] = child;
    child->parent = parent;
}

void ast_free(Ast_node *node) {
    if (!node) return;
    for (int i = 0; i < node->children_count; i++)
        ast_free(node->children[i]);
    free(node->children);
    free(node->id);
    free(node);
}
