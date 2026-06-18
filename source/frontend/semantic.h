#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/*
* On parcourt tout l'AST UNE SEULE FOIS et fait en meme temps :
*   - construire table_globale (voir symtable.h)
*   - verifier les regles de typage au fur et a mesure :
*     identifiants inconnus, nombre d'arguments d'un appel, structures
*     manipulées uniquement par pointeur, typage des opérateurs
*/

extern int sem_errors;

/* Analyse sémantique du programme entier : construit table_globale et
   incremente sem_errors a chaque erreur detectee (les messages sont
   ecrits directement sur stderr au fil de l'analyse). */
void sem_analyse(Ast_node *programme);

/* Libère table_globale (via symtable_liberer) et toutes les tables
   locales qu'elle contient. A appeler une fois la generation de code
   terminee (codegen.c lit table_globale sans jamais la liberer). */
void sem_liberer(void);

#endif
