/* Piege : x n'est jamais declare (ni en local ni en global), mais le seul
   controle d'identifiant inconnu fait par semantic.c porte sur les APPELS
   de fonction, pas sur les variables. On s'attend a ce que ceci compile
   "avec succes" alors que ca ne devrait pas -> revele un trou de verification. */
extern int printd(int i);

int main() {
    x = 5;
    printd(x);
    return 0;
}
