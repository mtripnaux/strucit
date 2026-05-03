
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "code.h"

// --- Constantes de formatage ---
#define COULEUR_ERREUR "31"    // Rouge
#define COULEUR_ALERTE "35"    // Magenta
#define STYLE_GRAS     "\033[1m"
#define STYLE_RESET    "\033[0m"

// --- Variables Globales ---
int profondeur = 0;      // Anciennement 'depth'
int compteur = 0;   // Anciennement 'bool_counter'

/**
 * Affiche les messages système (Erreurs/Avertissements) en français
 */
static void formater_log(const char *code_couleur, const char *titre, const char *patron, Symbol *symbole, char *identifiant, int ligne) {
    printf("\033[1;%sm%s\033[0m", code_couleur, titre);
    
    char *message;
    const char *genre_type;

    switch (symbole->type) {
        case FUNCTION_SYMBOL:   genre_type = "La fonction"; break;
        case IDENTIFIER_SYMBOL: genre_type = "L'identifiant"; break;
        default:                genre_type = "La structure"; break;
    }

    asprintf(&message, patron, genre_type, identifiant, ligne);
    printf("%s\n", message);
    free(message);
}

void print_error(Symbol *symbole, char *id, int ligne) {
    formater_log(COULEUR_ERREUR, "Erreur : ", "%s %s\"%s\"%s est déjà déclaré(e) (ligne %d)", symbole, id, STYLE_GRAS, id, STYLE_RESET, ligne);
}

void print_warning(Symbol *symbole, char *id, int ligne) {
    formater_log(COULEUR_ALERTE, "Avertissement : ", "Redéfinition de %s %s\"%s\"%s (ligne %d)", symbole, id, STYLE_GRAS, id, STYLE_RESET, ligne);
}

void print_color(char *couleur, char *texte) {
    printf("\033[1;%sm%s\033[0m", couleur, texte);
}

/**
 * Gère l'indentation (tabulations) selon la profondeur actuelle
 */
void ecrire_indentation(FILE *fichier) {
    for (int i = 0; i < profondeur; i++) {
        fprintf(fichier, "\t");
    }
}

/**
 * Inverse les opérateurs logiques pour la génération de sauts (gotos)
 */
char *inverser_operateur(char *op) {
    if (strcmp(op, "<") == 0)   return ">=";
    if (strcmp(op, "<=") == 0)  return ">";
    if (strcmp(op, ">") == 0)   return "<=";
    if (strcmp(op, ">=") == 0)  return "<";
    if (strcmp(op, "==") == 0)  return "!=";
    if (strcmp(op, "!=") == 0)  return "==";
    if (strcmp(op, "||") == 0)  return "&&";
    if (strcmp(op, "&&") == 0)  return "||";
    return NULL;
}

/**
 * Fonction principale de génération de code
 */
void write_code(Ast_node *noeud, FILE *fichier) {
    if (noeud == NULL) return;

    switch (noeud->type) {
        case AST_PROGRAM:
            for (int i = 0; i < noeud->children_count; i++) {
                write_code(noeud->children[i], fichier);
                if (noeud->children[i]->type == AST_DECLARATION) {
                    if (noeud->children[i]->children[0]->type != AST_STRUCT) {
                        fprintf(fichier, ";\n\n");
                    }
                } else {
                    fprintf(fichier, "\n");
                }
            }
            break;

        case AST_FUNCTION_DEFINITION:
            for (int i = 0; i < noeud->children_count; i++) {
                write_code(noeud->children[i], fichier);
                if (noeud->children[i]->type == AST_TYPE_SPECIFIER) {
                    fprintf(fichier, " ");
                }
            }
            fprintf(fichier, "\n");
            break;

        case AST_FOR: {
            int id_label = compteur_bool++;
            write_code(noeud->children[0], fichier);
            fprintf(fichier, ";\n");
            
            fprintf(fichier, "test_%d:\n", id_label);
            ecrire_indentation(fichier);
            
            // On prépare les labels pour la condition
            noeud->children[1]->true_label = id_label;
            noeud->children[1]->false_label = id_label;
            
            write_code(noeud->children[1], fichier);
            
            if (noeud->children[3]->type != AST_COMPOUND_STATEMENT) profondeur++;
            write_code(noeud->children[3], fichier);
            if (noeud->children[3]->type != AST_COMPOUND_STATEMENT) {
                fprintf(fichier, ";");
                profondeur--;
            }
            
            fprintf(fichier, "\n");
            ecrire_indentation(fichier);
            write_code(noeud->children[2], fichier);
            fprintf(fichier, ";\n");
            ecrire_indentation(fichier);
            fprintf(fichier, "goto test_%d;\nfalse_%d:", id_label, id_label);
            break;
        }

        case AST_WHILE: {
            int id_label = compteur_bool++;
            fprintf(fichier, "test_%d:\n", id_label);
            ecrire_indentation(fichier);
            noeud->children[0]->true_label = id_label;
            noeud->children[0]->false_label = id_label;
            
            write_code(noeud->children[0], fichier);
            write_code(noeud->children[1], fichier);
            fprintf(fichier, ";\ngoto test_%d;\nfalse_%d:", id_label, id_label);
            break;
        }

        case AST_COMPOUND_STATEMENT:
            ecrire_indentation(fichier);
            if (noeud->parent->type == AST_FUNCTION_DEFINITION) fprintf(fichier, "\n");
            fprintf(fichier, "{\n");
            profondeur++;
            for (int i = 0; i < noeud->children_count; i++) {
                write_code(noeud->children[i], fichier);
            }
            profondeur--;
            ecrire_indentation(fichier);
            fprintf(fichier, "}");
            break;

        case AST_UNARY_SIZEOF:
            // "taille de" / mesure
            fprintf(fichier, "%d /* octets */", noeud->size);
            break;

        case AST_BOOL_OP:
            fprintf(fichier, "if (");
            write_code(noeud->children[0], fichier);
            fprintf(fichier, " %s ", inverser_operateur(noeud->id));
            write_code(noeud->children[1], fichier);
            fprintf(fichier, ") goto false_%d;\n", noeud->false_label);
            break;

        // ... Autres cas ...
        case AST_IDENTIFIER:
            fprintf(fichier, "%s", noeud->id);
            break;

        case AST_CONSTANT:
            fprintf(fichier, "%d", noeud->value);
            break;

        default:
            for (int i = 0; i < noeud->children_count; i++) {
                write_code(noeud->children[i], fichier);
            }
            break;
    }
}
