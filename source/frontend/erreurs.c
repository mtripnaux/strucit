#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "erreurs.h"

const char *g_fichier_source = "<stdin>";

// Nombre total d'erreurs (lexicales + syntaxiques + semantiques) toutes
// categories confondues, tous compilateurs (frontend/backend) confondus.
// C'est ce compteur, et lui seul, qui doit decider si la compilation a
// echoue et qui doit etre annonce dans le message final : peu importe le
// melange exact de categories rencontrees, il n'y a qu'UN SEUL bilan.
int g_total_erreurs = 0;

static void entete(const char *categorie, int ligne) {
    fprintf(stderr, "%s (%s:%d) :\n    ", categorie, g_fichier_source, ligne);
}

void erreur_lexicale(int ligne, const char *caractere) {
    entete("Erreur lexicale", ligne);
    fprintf(stderr, "caractère '%s' inconnu\n", caractere);
    g_total_erreurs++;
}

void erreur_syntaxique(int ligne, const char *message) {
    entete("Erreur syntaxique", ligne);
    fprintf(stderr, "%s\n", message);
    g_total_erreurs++;
}

void erreur_semantique_v(int *compteur, int ligne, const char *fmt, va_list ap) {
    entete("Erreur sémantique", ligne);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    if (compteur) (*compteur)++;
    g_total_erreurs++;
}

void avertissement_semantique_v(int ligne, const char *fmt, va_list ap) {
    entete("Avertissement", ligne);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

void erreur_systeme(const char *contexte) {
    entete("Erreur système", 0);
    fprintf(stderr, "%s\n", contexte);
    fprintf(stderr, "    %s\n", strerror(errno));
}

void rapporter_echec_compilation(void) {
    fprintf(stderr, "\n[%s] Echec de la compilation (%d erreur(s))\n",
            g_fichier_source, g_total_erreurs);
}
