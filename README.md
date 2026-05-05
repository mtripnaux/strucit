# STRUCTIT - Compilateur

## 🚀 Démarrage rapide

### Compiler le projet
```bash
make
```
Génère `bin/structit`

### Tests automatiques

**Tests avec validation backend :**
```bash
make test-validate
```

**Tests simples :**
```bash
make test
```

Résultats dans `test_report.txt` et `test_output/`

### Tests manuels

Tester un fichier spécifique :
```bash
./bin/structit tests/add.c
```

Valider la syntaxe du code généré :
```bash
make backend
./bin/structit_backend add_3.c
```

## 📁 Structure

- `source/frontend/` - Lexer & parser du langage STRUCTIT-frontend
- `source/backend/` - Parser du langage STRUCTIT-backend
- `source/common/` - Structures communes (AST, symboles, génération de code)
- `tests/` - Fichiers de test en STRUCTIT-frontend
- `scripts/` - Scripts utilitaires (run_tests.sh)
- `bin/` - Exécutables générés
- `test_output/` - Sortie des tests (code STRUCTIT-backend)

## 🧪 Fichiers de test

Les fichiers dans `tests/` testent différentes fonctionnalités :
- `add.c`, `sub.c`, `mul.c`, `div.c` - Opérations arithmétiques
- `variables.c` - Déclarations et utilisation de variables
- `expr.c` - Expressions complexes
- `loops.c` - Boucles for/while
- `cond.c` - Conditionnelles if/else
- `functions.c` - Définition et appels de fonctions
- `pointeur.c` - Pointeurs
- `listes.c` - Structures et listes

## 🧹 Nettoyage

```bash
make clean          # Supprime les binaires
make clean-tests    # Supprime les résultats des tests
```

## 📝 Notes

- Les fichiers générés portent le suffixe `_3.c` (ex: `add_3.c`)
- Le rapport des tests se trouve dans `test_report.txt`
- Tous les tests sont dans le dossier `tests/`

## 🤖 CI/CD

Les tests s'exécutent automatiquement à chaque push via GitHub Actions.
L'icône de statut ✓ ou ✗ s'affiche à côté de chaque commit.

---

Auteurs : Bouchra Mezrhab, Mathéo Tripnaux-Moreau
