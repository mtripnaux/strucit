/* main() appelle foo() AVANT sa definition textuelle. Comme l'analyse
   semantique ne fait qu'un seul parcours lineaire de haut en bas (pas de
   pre-passe d'enregistrement des fonctions), foo n'est pas encore dans
   table_globale au moment de verifier l'appel : ca devrait donc, a tort,
   etre signale comme "fonction non declaree" alors que le programme est
   parfaitement valide (et gcc le compilerait sans probleme apres
   reordonnancement implicite... en fait non, gcc exigerait aussi un
   prototype, donc ce n'est pas absurde — mais le message d'erreur doit
   au moins etre correct et ne pas planter). */
extern int printd(int i);

int main() {
    printd(foo(5));
    return 0;
}

int foo(int x) {
    return x * 2;
}
