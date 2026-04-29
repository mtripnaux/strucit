Étape 5 — codegen.h / codegen.c (le plus gros)
Traverser l'AST produit par structfe.y et émettre du code STRUCIT-backend :

    Aplatir les expressions en temporaires _t1, _t2, …
    Convertir if/else → if (cond) goto L; ... Lelse:
    Convertir for/while → goto Ltest; Lbody: ... Ltest: if (...) goto Lbody;
    Effacer les structs → void * + arithmétique d'offset (p+4 pour le champ 2, etc.)
    Calculer sizeof statiquement depuis la table des structs
    Gérer les appels de fonctions (arguments aplatis en primaires)

Étape 6 — Makefile

make          # compile structit (frontend → backend)
make backend  # compile structit_backend (parseur de vérification)
make test     # teste tous les fichiers de tests/
make clean    # supprime les artefacts

Le Makefile appelle yacc -d, lex, puis gcc pour chaque outil.

Ordre conseillé quand tu reviens : étape 6 d'abord (10 min), puis étape 5 (le vrai travail, 2-3h).