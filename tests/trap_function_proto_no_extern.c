/* Un prototype de fonction sans "extern" et sans corps : syntaxiquement
   c'est juste une "declaration" ordinaire pour la grammaire (le
   declarateur a la forme d'une fonction, mais rien ne distingue ca d'une
   variable au niveau de la regle 'declaration'). semantic.c risque de
   l'enregistrer comme une simple variable IDENTIFIER_SYMBOL au lieu d'un
   FUNCTION_SYMBOL, ce qui casserait l'appel plus bas silencieusement
   (mauvais type, ou codegen qui genere "int foo;" au lieu d'un prototype). */
extern int printd(int i);

int foo(int n);

int main() {
    printd(foo(5));
    return 0;
}
