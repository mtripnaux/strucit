#include "symbol.h"
 
Symbol *creer_symbole(char *id, int taille, Symbol_type type) {
    Symbol *s = malloc(sizeof(Symbol));
    s->id = id ? strdup(id) : NULL;
    s->size = taille;
    s->offset = 0;
    s->child_count = 0;
    s->children = NULL;
    s->type = type;
    s->type_name = NULL;
    s->struct_name = NULL;
    s->pointer = false;
    return s;
}

void ajouter_symbole_enfant(Symbol *parent, Symbol *enfant) {
    parent->child_count++;
    parent->children = realloc(parent->children,
                               sizeof(Symbol *) * parent->child_count);
    parent->children[parent->child_count - 1] = enfant;
}

Symbol *chercher_symbole_enfant(Symbol *parent, char *cle) {
    if (!parent || !cle) return NULL;
    for (int i = 0; i < parent->child_count; i++)
        if (parent->children[i]->id &&
            strcmp(parent->children[i]->id, cle) == 0)
            return parent->children[i];
    return NULL;
}

static Ast_node *first_identifier(Ast_node *decl) {
    if (!decl) return NULL;
    if (decl->type == AST_IDENTIFIER) return decl;
    if (decl->children_count > 0) return first_identifier(decl->children[0]);
    return NULL;
}

void extraire_arguments_fonction(Ast_node *noeud, Symbol *symbole) {
    if (!noeud || noeud->type != AST_PARAM_LIST) return;
    for (int i = 0; i < noeud->children_count; i++) {
        Ast_node *param = noeud->children[i];
        if (param->type != AST_PARAM || param->children_count < 2) continue;
        Ast_node *type_spec = param->children[0];
        Ast_node *decl = param->children[1];
        Ast_node *name_nd = first_identifier(decl);
        char *name = name_nd ? name_nd->id : "_anon";
        Symbol *ps = creer_symbole(name, 4, IDENTIFIER_SYMBOL);
        if (type_spec->type == AST_TYPE_SPECIFIER)
            ps->type_name = strdup(type_spec->id);
        else if (type_spec->type == AST_STRUCT) {
            ps->type_name = strdup("struct");
            if (type_spec->children_count > 0)
                ps->struct_name = strdup(type_spec->children[0]->id);
            ps->pointer = true;
        }
        if (decl->type == AST_STAR_DECLARATOR) ps->pointer = true;
        ajouter_symbole_enfant(symbole, ps);
    }
}

void ajouter_symbole_retour(Ast_node *noeud, Symbol *symbole) {
    if (!noeud) return;
    if (noeud->type == AST_TYPE_SPECIFIER)
        symbole->type_name = strdup(noeud->id);
    else if (noeud->type == AST_STRUCT) {
        symbole->type_name = strdup("struct");
        if (noeud->children_count > 0)
            symbole->struct_name = strdup(noeud->children[0]->id);
        symbole->pointer = true;
    }
}
