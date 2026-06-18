#ifndef ERREURS_H
#define ERREURS_H

#include <stdarg.h>

/* Chemin du fichier source en cours de traitement : a affecter (main())
   avant toute analyse lexicale/syntaxique/semantique. Utilise comme
   prefixe par toutes les fonctions de ce module, pour que chaque message,
   quelle que soit sa categorie ou le binaire (frontend ou backend) qui
   l'emet, ait exactement le meme format :
       Categorie (fichier:ligne) :
           message
   (numero de ligne 0 pour une erreur systeme, qui n'en a pas vraiment). */
extern const char *g_fichier_source;

/* Nombre total d'erreurs (lexicales + syntaxiques + semantiques) depuis le
   debut de la compilation, toutes categories confondues. C'est lui qu'il
   faut consulter pour savoir si la compilation a echoue, et c'est lui que
   rapporte rapporter_echec_compilation() : un seul bilan general, jamais
   un compte par categorie. */
extern int g_total_erreurs;

/* "Erreur lexicale (fichier:ligne) : caractère 'X' inconnu" */
void erreur_lexicale(int ligne, const char *caractere);

/* "Erreur syntaxique (fichier:ligne) : message" */
void erreur_syntaxique(int ligne, const char *message);

/* "Erreur sémantique (fichier:ligne) : message" (printf-style). Si compteur
   n'est pas NULL, il est aussi incremente (c'est ainsi que sem_errors,
   dans semantic.c, reste a jour sans que ce module ait besoin de connaitre
   son existence) ; g_total_erreurs est toujours incremente en plus. */
void erreur_semantique_v(int *compteur, int ligne, const char *fmt, va_list ap);

/* "Avertissement (fichier:ligne) : message" (printf-style). N'incremente
   ni compteur ni g_total_erreurs : un avertissement n'arrete jamais la
   compilation. */
void avertissement_semantique_v(int ligne, const char *fmt, va_list ap);

/* "Erreur système (fichier:0) : contexte / description issue de errno" —
   pour les echecs d'appel systeme (ouverture/creation de fichier, etc).
   N'incremente pas g_total_erreurs : c'est un echec immediat et isole, pas
   une erreur qui s'accumule avec d'autres avant un bilan final. */
void erreur_systeme(const char *contexte);

/* Message final unique, normalise, valable pour n'importe quel melange de
   categories d'erreurs : "fichier : echec de la compilation (N erreur(s))".
   A appeler une seule fois, juste avant de quitter en echec, que l'echec
   vienne de la phase lexicale/syntaxique ou de la phase semantique. */
void rapporter_echec_compilation(void);

#endif
