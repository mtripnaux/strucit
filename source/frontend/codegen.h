#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "symbol.h"

//afficher erreur de declaration
void print_error(Symbol *symbole, char *id, int ligne);
//afficher avertissement 
void print_warning(Symbol *symbole, char *id, int ligne);
//texte en couleur dans le terminal 
void print_color(char *couleur, char *texte);

//gerer tabukation
void ecrire_indentation(FILE *fichier);
//inversement des operateurs logiques 
char *inverser_operateur(char *op);
//pour parcourir l arbre AST et ecrire le code traduit dans le fichier 
void write_code(Ast_node *noeud, FILE *fichier);

#endif
