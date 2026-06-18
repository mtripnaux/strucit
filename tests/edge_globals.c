/* Stress: variables globales (non-extern) et externes, partagees entre fonctions */
extern int printd(int i);

int compteur;
extern int constante_externe;

void incremente() {
    compteur = compteur + 1;
}

int main() {
    compteur = 0;
    incremente();
    incremente();
    incremente();
    printd(compteur);    /* 3 */
    return 0;
}
