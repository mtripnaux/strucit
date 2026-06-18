# STRUCIT: Extension de projet

Liste de ce qui a été fait depuis le début de la refonte.

## Bugs corrigés

- Le code généré pour une comparaison utilisée comme valeur (`r = a < b;`) utilisait un opérateur ternaire `? :` qui n'existe pas dans le langage STRUCIT-backend
- Les constantes négatives repliées (`-5`) étaient parfois insérées directement comme opérande d'une opération binaire, d'un appel de fonction ou d'une affectation via pointeur (`*p = ...`) alors que la grammaire backend exige un `primary_expression` nu à ces endroits
- Le code généré contenait des accolades `{ }` autour des blocs `if`/`while`/`for`, alors que le code trois-adresses du backend doit être entièrement aplati
- Chaque label généré était systématiquement suivi d'un `;` vide inutile
- Une fonction dont un paramètre est un pointeur voyait son **type de retour** basculer à tort en `void *`
- `sizeof(p)` retournait toujours 4 au lieu de la vraie taille de la structure pointée
- Les fonctions retournant `void *` (ex. `malloc`) étaient parfois traitées comme des fonctions `void` à cause d'une vérification de type incomplète
- Une variable de type pointeur de fonction (`int (*op)(int n);`) n'était pas reconnue comme pointeur dans le code généré
- Deux tableaux à taille fixe produisaient silencieusement du code invalide

## Vérifications sémantiques

- Une structure ne peut être manipulée que par pointeur (variable, champ, paramètre, type de retour)
- Règles de typage des opérateurs : `+`/`-`/`*`/`/` sur des pointeurs restreints aux cas autorisés (pointeur +- entier, pointeur - pointeur) ; `*`, `->`, `&`, `-` unaires restreints à leur type attendu

## Nettoyage du code

- Suppression de code mort : `create_node()`, champs `Ast_node->parent`/`->size`, `ajouter_symbole_retour()`, `extraire_arguments_fonction()`
- Fusion de blocs de code dupliqués dans `codegen.c`
- Le générateur de code (`codegen.c`) ne reconstruit plus sa propre table de symboles : il réutilise celle déjà construite et vérifiée par l'analyse sémantique (`semantic.c`), via `symtable.c`. Ça supprime la source des bugs ci-dessus liés à deux implémentations parallèles qui divergeaient
- `Makefile` : la cible `backend` liait inutilement `ast.c`/`symbol.c`/`codegen.c`, jamais utilisés par le parseur backend
- Suppression des déclarations `%left`/`%right` inutiles et des règles `[`/`]` mortes du lexer frontend

## Comparaison avec l'énoncé

- Confirmé que la grammaire frontend de référence contient un conflit shift/reduce (dangling-else) que notre grammaire corrige bien (vérifié avec bison, 0 conflit des deux côtés).
- Grammaire backend : copie fidèle de la référence.
- Identifiants backend autorisés à commencer par `_` (nécessaire pour enlever l'ambiguité sur les temporaires `_temp_N`)

## Tests

Beaucoup de nouveaux tests.