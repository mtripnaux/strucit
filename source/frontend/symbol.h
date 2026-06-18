#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum _Symbol_type {
   FUNCTION_SYMBOL,    // une fonction (ses ->children sont params + return)
   IDENTIFIER_SYMBOL,  // une variable, un parametre, ou un champ de structure
   STRUCT_SYMBOL,      // une definition de structure (ses ->children sont ses champs)
} Symbol_type;

typedef struct _Symbol {
   char *id; // nom du symbole (nom de variable/fonction/struct/champ)
   int size; // taille en octets
   int offset; // position en octets dans la structure parente (champ uniquement)
   int child_count;
   struct _Symbol **children;
   Symbol_type type;
   char *type_name; // "int", "void", ou "struct"
   char *struct_name; // si pointeur sur structure : nom de la structure
   bool pointer;
   struct _Symbol *locales; // Pour FUNCTION_SYMBOL, variables locales
} Symbol;

/* Alloue un symbole vide (aucun enfant, pointer=false, type_name/struct_name
   NULL). 'taille' initialise ->size directement. */
Symbol *creer_symbole(char *id, int taille, Symbol_type type);

/* Ajoute enfant a la fin de la liste des enfants de parent. */
void ajouter_symbole_enfant(Symbol *parent, Symbol *enfant);

/* Recherche un enfant direct de parent par son nom (->id), recherche
   lineaire. Renvoie NULL si non trouve ou si parent/cle est NULL. */
Symbol *chercher_symbole_enfant(Symbol *parent, char *cle);

/* Libere recursivement un symbole, ses enfants, et sa table ->locales
   eventuelle. */
void liberer_symbole(Symbol *s);

#endif
