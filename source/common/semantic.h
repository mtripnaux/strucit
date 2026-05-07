#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol.h"

/* Table des symboles globale et locale */
extern Symbol *sem_global;
extern Symbol *sem_local;
extern int     sem_errors;

/* Initialisation */
void sem_init(void);

/* Analyse sémantique du programme entier */
void sem_analyse(Ast_node *programme);

#endif
