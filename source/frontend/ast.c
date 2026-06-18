#include "ast.h"

static Ast_node *alloc_node(Ast_type type) {
    Ast_node *n = malloc(sizeof(Ast_node));
    n->type = type;
    n->id = NULL;
    n->value = 0;
    n->line = 0;
    n->children = NULL;
    n->children_count = 0;
    return n;
}

Ast_node *ast_create_node(Ast_type type) {
    return alloc_node(type);
}

Ast_node *create_constant_leaf(int value) {
    Ast_node *n = alloc_node(AST_CONSTANT);
    n->value = value;
    return n;
}

Ast_node *create_identifier_leaf(char *name) {
    Ast_node *n = alloc_node(AST_IDENTIFIER);
    n->id = strdup(name); // copie
    return n;
}

void ast_add_child(Ast_node *parent, Ast_node *child) {
    if (!parent || !child) return;
    parent->children_count++;
    parent->children = realloc(
        parent->children,
        sizeof(Ast_node *) * parent->children_count
    ); // Realloc sans limite sur le nombre d'enfants
    parent->children[parent->children_count - 1] = child;
}

void ast_free(Ast_node *node) {
    if (!node) return;
    for (int i = 0; i < node->children_count; i++)
        ast_free(node->children[i]);
    free(node->children);
    free(node->id);
    free(node);
}

// N'examine que children[0] a chaque niveau : c'est la "chaine principale"
// du declarateur (ex: pour int (*f)(int n), elle passe par STAR_DECLARATOR
// puis l'identifiant f). children[1] d'un AST_FUNC_DECLARATOR est la liste
// de parametres : il ne faut JAMAIS y descendre ici, sinon un parametre
// pointeur ferait passer à tort la fonction elle-meme pour un pointeur
int ast_est_pointeur(Ast_node *decl) {
    if (!decl) return 0;
    if (decl->type == AST_STAR_DECLARATOR) return 1;
    if (decl->children_count > 0) return ast_est_pointeur(decl->children[0]);
    return 0;
}

Ast_node *ast_nom_declarateur(Ast_node *decl) {
    if (!decl) return NULL;
    if (decl->type == AST_IDENTIFIER) return decl;
    if (decl->children_count > 0) return ast_nom_declarateur(decl->children[0]);
    return NULL;
}

// Recherche large (tous les enfants, pas seulement children[0]) : utile
// quand on ne connait pas a priori la forme exacte du sous-arbre, par
// exemple pour retrouver le nom d'une struct dans son noeud de type.
Ast_node *ast_premier_identifiant(Ast_node *n) {
    if (!n) return NULL;
    if (n->type == AST_IDENTIFIER) return n;
    for (int i = 0; i < n->children_count; i++) {
        Ast_node *r = ast_premier_identifiant(n->children[i]);
        if (r) return r;
    }
    return NULL;
}

Ast_node *ast_decl_fonction(Ast_node *decl) {
    if (!decl) return NULL;
    if (decl->type == AST_FUNC_DECLARATOR || decl->type == AST_DIRECT_DECLARATOR) return decl;
    if (decl->children_count > 0) return ast_decl_fonction(decl->children[0]);
    return NULL;
}

Ast_node *ast_liste_parametres(Ast_node *decl) {
    Ast_node *fd = ast_decl_fonction(decl);
    if (fd && fd->type == AST_FUNC_DECLARATOR && fd->children_count >= 2)
        return fd->children[1];
    return NULL;
}
