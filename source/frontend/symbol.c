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
    s->locales = NULL;
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

void liberer_symbole(Symbol *s) {
    if (!s) return;
    for (int i = 0; i < s->child_count; i++)
        liberer_symbole(s->children[i]);
    liberer_symbole(s->locales);
    free(s->children);
    free(s->id);
    free(s->type_name);
    free(s->struct_name);
    free(s);
}
