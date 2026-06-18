#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "symbol.h"

//gerer tabukation
void ecrire_indentation(FILE *fichier);
//inversement des operateurs logiques 
char *inverser_operateur(char *op);
//pour parcourir l arbre AST et ecrire le code traduit dans le fichier
void write_code(Ast_node *noeud, FILE *fichier);

// libère les tables de symboles et tableaux de temporaires du codegen
void codegen_liberer(void);

#endif
