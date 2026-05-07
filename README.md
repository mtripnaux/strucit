# Mini Compilateur C — STRUCIT

## Présentation

Ce projet est un mini compilateur qui traduit du code C simplifié (**STRUCIT-frontend**) vers un langage intermédiaire de type assembleur (**STRUCIT-backend**), en générant du code à trois adresses.

Le compilateur effectue trois phases :
- **Analyse lexicale** : reconnaissance des tokens (flex)
- **Analyse syntaxique** : vérification de la grammaire (bison)
- **Analyse sémantique** : vérification des types, variables déclarées, arguments de fonctions

---

## Prérequis

```bash
sudo apt-get install flex bison gcc
```

---

## Structure du projet

```
strucit/
│
├── source/
│   ├── frontend/        # Lexer et grammaire du langage source (ANSI-C.l, structfe.y)
│   ├── backend/         # Lexer et grammaire du langage cible (ANSI-BE.l, structbe.y)
│   └── common/          # AST, table des symboles, analyse sémantique, génération de code
│
├── tests/               # Fichiers de test en STRUCIT-frontend
├── output/              # Fichiers générés après compilation (créé automatiquement)
├── examples/            # Exemples de code frontend et backend
├── bin/                 # Exécutables (créé automatiquement par make)
└── Makefile
```

---

## Compilation

**Compiler le front-end et le back-end :**
```bash
make && make backend
```

**Nettoyer les fichiers générés :**
```bash
make clean
```

---

## Utilisation

### Compiler un fichier source

```bash
./bin/structit <fichier_source.c> <fichier_sortie.c>
```

Exemple :
```bash
mkdir -p output
./bin/structit tests/add.c output/add_backend.c
```

Le compilateur affiche :
- `Starting compilation...` — début de la compilation
- `Compilation finished :)` — succès
- Les erreurs sémantiques détectées le cas échéant

### Vérifier le fichier backend généré

```bash
./bin/structit_backend < output/add_backend.c
```

Si le fichier est valide, le programme affiche :
```
Analyse syntaxique backend : OK
```

---

## Exemples de tests

```bash
./bin/structit tests/add.c output/add_backend.c
./bin/structit_backend < output/add_backend.c

./bin/structit tests/loops.c output/loops_backend.c
./bin/structit_backend < output/loops_backend.c

./bin/structit tests/cond.c output/cond_backend.c
./bin/structit_backend < output/cond_backend.c
```

---

## Gestion des erreurs

Le compilateur détecte et signale les erreurs suivantes :

**Erreurs lexicales et syntaxiques** — tout token ou construction non conforme à la grammaire STRUCIT-frontend est rejeté avec le numéro de ligne concerné.

**Erreurs sémantiques** — les cas suivants sont détectés et arrêtent la compilation :
- Variable ou identifiant non déclaré
- Appel de fonction avec un mauvais nombre d'arguments
- Fonction sans déclaration `extern` préalable

Exemple de message d'erreur :
```
Error: Unknown identifier "x" (line 5)
Error: Function "foo" requires 2 arguments but 3 were given (line 12)
```

---

## Compilation manuelle (si le Makefile ne fonctionne pas)

**Créer les dossiers nécessaires :**
```bash
mkdir -p bin output
```

**Compiler le front-end :**
```bash
bison -d -o source/frontend/structfe.tab.c source/frontend/structfe.y
flex -o source/frontend/lex.yy.c source/frontend/ANSI-C.l
gcc -Wall -g -I./source/common -I./source/frontend -I./source/backend \
    source/frontend/structfe.tab.c \
    source/frontend/lex.yy.c \
    source/common/ast.c \
    source/common/symbol.c \
    source/common/codegen.c \
    source/common/semantic.c \
    -o bin/structit -lfl
```

**Compiler le back-end :**
```bash
bison -d -o source/backend/structbe.tab.c source/backend/structbe.y
flex -o source/backend/lex.be.c source/backend/ANSI-BE.l
gcc -Wall -g -I./source/common -I./source/frontend -I./source/backend \
    source/backend/structbe.tab.c \
    source/backend/lex.be.c \
    source/common/ast.c \
    source/common/symbol.c \
    source/common/codegen.c \
    -o bin/structit_backend -lfl
```
