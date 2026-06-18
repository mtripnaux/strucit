#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "symbol.h"

/* Table des symboles globale du programme (fonctions, variables globales,
   structures). Construite et verifiee par l'analyse semantique, puis lue
   par le generateur de code : ni l'un ni l'autre n'en est "proprietaire",
   d'ou ce module a part plutot qu'une dependance de codegen vers semantic. */
extern Symbol *table_globale;

void symtable_creer(void);
void symtable_liberer(void);

#endif
