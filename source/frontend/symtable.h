#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "symbol.h"

/*
 * table_globale contient un Symbol pour chaque fonction, variable globale
 * et structure du fichier source. Elle est construite une seule fois, par
 * semantic.c pour l'analyse et lue par codegen.c pour generer le code.
 */
extern Symbol *table_globale;

/* Alloue la table globale (vide). A appeler avant toute analyse. */
void symtable_creer(void);

/* Libere recursivement la table globale (et donc, via Symbol->locales,
   toutes les tables locales de chaque fonction). */
void symtable_liberer(void);

#endif
