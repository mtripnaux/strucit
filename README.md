# Mini Compilateur C: STRUCIT

## Présentation

Ce projet est un mini compilateur qui traduit du code C simplifié (**STRUCIT-frontend**) vers un langage intermédiaire de type assembleur (**STRUCIT-backend**), en générant du code à trois adresses.

Le compilateur effectue trois phases :
- **Analyse lexicale** : reconnaissance des tokens (flex)
- **Analyse syntaxique** : verification de la grammaire (bison)
- **Analyse sémantique** : vérification des types, variables déclarées, arguments de fonctions

---

## Prérequis

```bash
sudo apt-get install flex bison gcc
```


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
- `compilation commencee<3` début de la compilation
- `Compilation finie<3`  succès
- Les erreurs sémantiques détectées le cas échéant

### Verifier le fichier backend genere

```bash
./bin/structit_backend < output/add_backend.c
```

Si le fichier est valide, le programme affiche :
```
Analyse syntaxique perfecto
```

---

## Exemples de tests

```bash
./bin/structit tests/add.c output/add_backend.c
./bin/structit_backend < output/add_backend.c

./bin/structit tests/loops.c output/loops_backend.c
./bin/structit_backend < output/loops_backend.c
```

---

## Gestion des erreurs

Le compilateur détecte et signale les erreurs suivantes :

**Erreurs lexicales et syntaxiques** tout token ou construction non conforme à la grammaire STRUCIT-frontend est rejeté avec le numéro de ligne concerné.

**Erreurs sémantiques**  les cas suivants sont détectés et arrêtent la compilation :
- Variable ou identifiant non déclaré
- Appel de fonction avec un mauvais nombre d'arguments
- Fonction sans déclaration `extern` préalable

Exemple de message d'erreur :

```
Error: identifiant inconnu "x" (ligne y)
```