#ifndef SYMBOL_H
#define SYMBOL_H
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ast.h"

typedef enum _Symbol_type
{
    FUNCTION_SYMBOL,
    IDENTIFIER_SYMBOL,
    STRUCT_SYMBOL,
} Symbol_type;

typedef struct _Symbol
{
    char *id;

    // Structure et Mémoire
    int size;
    int offset;

    int child_count;
    struct _Symbol **children;

    Symbol_type type;
    char *type_name;

    char *struct_name;

    // Gestion d'erreurs
    bool pointer;
} Symbol;


Symbol *creer_symbole(char *id, int taille, Symbol_type type);

void ajouter_symbole_enfant(Symbol *parent, Symbol *enfant);

Symbol *chercher_symbole_enfant(Symbol *parent, char *cle);

void extraire_arguments_fonction(Ast_node *noeud, Symbol *symbole);

void ajouter_symbole_retour(Ast_node *noeud, Symbol *symbole);

#endif
