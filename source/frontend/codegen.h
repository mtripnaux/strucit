#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "symbol.h"

/*
 * table_globale contient deja tout ce dont ce fichier a besoin, codegen.c
 * ne fait que LIRE cette table, jamais la modifier ni la reconstruire.
 * toute expression est traduite en une suite d'instructions t = a op b
 * qui n'utilisent que des temporaires/variables/constantes et renvoie le  
 * nom du temporaire qui contient son resultat. Les if/while/for -> gotos/labels
 */

/* Ecrit l'indentation courante sur fichier, puis les labels actuellement
   "en attente" (cf marquer_label dans codegen.c) en prefixe de la
   prochaine instruction. A appeler avant CHAQUE ligne de sortie generee. */
void ecrire_indentation(FILE *fichier);

/* Operateur de comparaison inverse (< devient >=, etc.) : utilise pour
   traduire "if (!cond) goto L" en un saut direct sur la condition
   inversee, sans avoir besoin d'un operateur de negation dans le backend. */
char *inverser_operateur(char *op);

/* Parcourt tout l'AST (prog) et ecrit le code STRUCIT-backend correspondant
   dans fichier. A appeler une seule fois, apres une analyse semantique
   reussie (sem_errors == 0). */
void write_code(Ast_node *noeud, FILE *fichier);

/* Libère les tableaux de temporaires/labels internes a codegen.c. Ne
   touche PAS a table_globale (cf symtable_liberer, appele separement par
   sem_liberer) : ce fichier ne possede pas cette table, il ne fait que la
   lire. */
void codegen_liberer(void);

#endif
