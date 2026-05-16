# Mini Compilateur C — STRUCIT

## Presentation

Ce projet est un mini compilateur qui traduit du code C simplifie (**STRUCIT-frontend**) vers un langage intermediaire de type assembleur (**STRUCIT-backend**), en generant du code a trois adresses.

Le compilateur effectue trois phases :
- **Analyse lexicale** : reconnaissance des tokens (flex)
- **Analyse syntaxique** : verification de la grammaire (bison)
- **Analyse semantique** : verification des types, variables declarees, arguments de fonctions

---

## Prerequis

```bash
sudo apt-get install flex bison gcc
```


## Compilation

**Compiler le front-end et le back-end :**
```bash
make && make backend
```

**Nettoyer les fichiers generes :**
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
- `compilation commencee<3` debut de la compilation
- `Compilation finie<3`  succes
- Les erreurs semantiques detectees le cas echeant

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

---

## Gestion des erreurs

Le compilateur detecte et signale les erreurs suivantes :

**Erreurs lexicales et syntaxiques** tout token ou construction non conforme a la grammaire STRUCIT-frontend est rejete avec le numero de ligne concerne.

**Erreurs semantiques**  les cas suivants sont detectes et arretent la compilation :
- Variable ou identifiant non declare
- Appel de fonction avec un mauvais nombre d arguments
- Fonction sans declaration `extern` prealable

Exemple de message d'erreur :
```
Error: identifiant inconnu "x" (line y)
