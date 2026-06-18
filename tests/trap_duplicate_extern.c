/* La meme fonction externe declaree deux fois avec des arites differentes :
   verifie que la table des symboles ne se retrouve pas avec deux entrees
   incoherentes pour le meme nom (la deuxieme ecrase silencieusement la
   premiere ? les deux coexistent et l'appel matche la mauvaise ?). */
extern int printd(int i);
extern int printd(int i);

int main() {
    printd(42);
    return 0;
}
