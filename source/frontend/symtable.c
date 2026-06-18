#include "symtable.h"

Symbol *table_globale = NULL;

void symtable_creer(void) {
    table_globale = creer_symbole("__global__", 0, FUNCTION_SYMBOL);
}

void symtable_liberer(void) {
    liberer_symbole(table_globale);
    table_globale = NULL;
}
