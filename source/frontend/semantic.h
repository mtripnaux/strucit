#ifndef SEMANTIC_H
#define SEMANTIC_H
 
#include "ast.h"

extern int sem_errors;

/* Analyse sémantique du programme entier */
void sem_analyse(Ast_node *programme);

/* Libère les tables de symboles sémantiques */
void sem_liberer(void);

#endif
