# Introduction
# Remise à niveau

Un de mes objectifs pour cette seconde chance est de produire une base de code propre et lisible, pour éviter la confusion qui était la mienne. Je compte notamment passer beaucoup plus de temps à comprendre chaque étape. Pour cette raison, j'ai décidé de commencer par simplifier au maximum le code produit pour que le projet se limite strictement aux attendues.

## Simplification

Lors de la première itération, Bouchra et moi-même avions passé beaucoup de temps sur l'optimisation et sur des détails qui nous semblaient intéressants, sans prêter assez d'attention aux fondements de ce qui nous était demandé. Nous avions notamment implémenté l'algorithme de Sethi-Ullman pour la minimisation des registres alors même que la génération de code trois-adresses n'était pas maîtrisée. J'ai décidé de supprimer cette partie pour me concentrer uniquement sur la production de code valide dans tous les cas possibles.

## Architecture
## Problèmes

La prochaine étape est naturellement de résoudre tous les problèmes mentionnés pendant la présentation de la première version, ainsi que d'autres remarqués pendant la refonte du projet.

### Expressions booléennes

L'erreur la plus critique était la production par notre générateur d'un code STRUCIT-backend incorrect, qui n'était pas reconnu par notre propre parseur. Le générateur produisait une opération ternaire pour caster la valeur des expressions booléennes (ex. comparaison entre entiers) vers le type entier à l'intérieur des temporaires, ce qui, en plus d'être totalement en dehors du langage cible STRUCIT-frontend, est inutile puisque les valeurs booléennes en C sont déjà des entiers.  

Cette partie du code avait été produite d'une traite pendant un rendez-vous pour travailler sur le projet et n'a jamais été relue ni modifiée ensuite, ce qui est révélateur de notre manque de rigueur et de méthodologie. Dans la version actuelle, en plus d'avoir réécrit cette partie du code, je m'engage à travailler autrement en écrivant des tests unitaires pour chaque fonctionnalité que j'implémente et en relisant systématiquement tout mon code et le résultat des tests avant chaque soumission.

### Blocs conservés

Les blocs, délimités par des accolades en C, ne font pas partie du langage STRUCIT-frontend et ne devraient donc pas être conservés, ce qui était le cas dans la première version. Ce changement est mineur et sera conservé pour la génération de code assembleur.

### Labels et points-virgules

Les labels, de la forme `NomDuLabel:`, étaient systématiquement complétés par des points-virgules à cause de ????. La solution est simple, attacher la prochaine instruction au label si elle existe et mettre un point-virgule sinon. Ce changement est mineur et ne sera pas conservé, car l'assembleur [NOM_ASM] de permet pas l'écriture de plusieurs instructions sur une ligne et n'inclut donc pas de caractère séparateur d'instructions comme le point-virgule en C.

### Erreur syntaxique if(a > b)

### Validation sémantique

L'énoncé impose que les structures ne puissent être *allouées* que par une fonction de type `malloc`. Cette contrainte n'était pas vérifiée. Une analyse statique complète serait interprocédurale (impossible sans analyse de flux), mais une vérification utile et sans faux positif est réalisable : si une variable de type pointeur-sur-structure reçoit le résultat d'un appel à une fonction connue dans la table des symboles dont le type de retour est `int` ou `void` (donc pas un pointeur), alors cette fonction ne peut pas avoir alloué la structure et l'affectation est fausse. Les fonctions qui retournent un pointeur (`void *`, comme `malloc`) et les pointeurs de fonction passés en paramètre sont tolérés. Cette vérification est implémentée dans `verifier_expression` au cas `AST_ASSIGNMENT` de `semantic.c`.

# Nouveautés

## Gestion des erreurs

## Reprise sur erreur

# Assembleur

# Résultats

# Sentiments

# Conclusion