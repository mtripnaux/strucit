/* Beaucoup d'erreurs distinctes sur une seule ligne : verifie que la
   recuperation panic-mode (un seul resync par "error ';'") les compte
   correctement sans boucler ni planter. */
extern int printd(int i);

int main() { int a; a = ; a = ) ; a = + + ; a = * / ; printd(a); return 0; }
