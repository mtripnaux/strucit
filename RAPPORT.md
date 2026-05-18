# Rapport d'implémentation

## Problèmes de fond rencontrés

### 1. Sethi-Ullman : ordre d'évaluation vs réutilisation de registres

L'algorithme Sethi-Ullman minimise le nombre de registres *simultanément actifs* en ordonnant l'évaluation des sous-arbres (lourd en premier). Dans un code trois adresses, cela se traduit par un ordre d'instructions optimal. Mais la réutilisation des temporaires entre instructions relève de l'allocation de registres, un problème distinct. L'énoncé demande Sethi-Ullman sans distinguer les deux : on a dû implémenter les deux ensemble pour que le résultat soit réellement efficace.

---

### 2. Représentation des types : tout aplati en `int` / `void *`

L'énoncé distingue `int`, `void`, pointeurs sur struct, pointeurs sur int, pointeurs sur fonctions. En backend, tous les pointeurs deviennent `void *`. Cette simplification est conforme à l'énoncé (qui l'impose) mais a des conséquences : l'information de type est perdue dès la génération de code, la vérification sémantique du backend devient impossible, et les appels de fonctions via pointeur nécessitent des appels sans cast (`void *` appelé directement).

---

### 3. `sizeof` : taille du pointeur ou de la structure pointée

L'énoncé dit : *"sizeof retourne la taille en octets de la structure pointée par la variable"*. En C standard, `sizeof(p)` pour un pointeur retourne 4 (taille du pointeur). On a implémenté la sémantique C standard. Pour les exemples de l'énoncé, `sizeof(p)` avec `p` de type `struct liste *` génère 4, alors que la structure fait 8 : les deux valeurs apparaissent dans les exemples de l'énoncé selon le contexte, ce qui rend la spécification ambiguë.

---

### 4. Écarts entre `ANSI-C.l` fourni et la grammaire de l'énoncé

Le fichier `ANSI-C.l` fourni est un squelette ANSI C complet (`/* A compléter */` partout) qui contient des tokens absents de la grammaire STRUCIT-frontend : `<<`, `>>`, `++`, `--`. Tous sont à ignorer selon l'énoncé, mais retourner ces tokens depuis le lexer les rendait implicitement utilisables. On a supprimé `<<`/`>>` des deux lexers et grammaires. Pour `++`/`--`, les règles de grammaire ont été retirées : le lexer retourne toujours le token mais le parser échoue immédiatement. L'énoncé fournit aussi `postfix '.' IDENTIFIER` pour l'accès direct aux champs de struct — absent de notre grammaire jusqu'ici, ajouté pour conformité.

---

### 5. Analyse sémantique : périmètre non spécifié

L'énoncé demande de signaler les erreurs si le code est incorrect, sans préciser le périmètre. On a implémenté la vérification des fonctions non déclarées et du nombre d'arguments. La vérification des types dans les expressions (opérations arithmétiques sur pointeurs, passage de pointeur là où un `int` est attendu) n'a pas été faite (ce serait nécessaire pour un vrai compilateur).

---

## Tests ajoutés

Les tests fournis par l'énoncé couvrent les fonctionnalités principales mais ne testent pas les cas d'erreur. On a ajouté :

**Tests valides (doivent compiler) :**

- `ptr.c` : manipulation de pointeurs entiers avec `malloc`, déréférencement et arithmétique. Remplace fonctionnellement `pointeur.c` qui est conservé tel quel mais ne peut pas passer (voir ci-dessous).

**Tests d'erreur (doivent être rejetés) :**

- `expr.c` (fourni) : utilise `<<` et `>>`, absents de la grammaire de l'énoncé. Classé comme erreur attendue.
- `pointeur.c` (fourni) : utilise `malloc` sans `extern` et l'opérateur `++` non supporté. Conservé tel quel, classé comme erreur attendue.
- `err_increment.c` : utilise `i++` : vérifie que l'opérateur `++` est bien rejeté avec un message explicite.
- `err_args.c` : appelle `add(1)` alors que `add` est déclaré avec deux paramètres : vérifie la détection du mauvais nombre d'arguments.
- `err_undeclared.c` : appelle `foo()` sans déclaration préalable : vérifie la détection des identifiants inconnus.
