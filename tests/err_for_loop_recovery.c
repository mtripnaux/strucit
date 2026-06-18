/* Erreur syntaxique a l'interieur d'une clause de for : la recuperation
   (resync sur le ';' suivant) risque de "manger" un mauvais bout du for.
   On verifie juste que ca n'explose pas et que ca affiche au moins une
   erreur coherente, meme si la suite du fichier devient incomprehensible. */
extern int printd(int i);

int main() {
    int i;
    for (i = 0; i < ) ; i = i + 1) {
        printd(i);
    }
    return 0;
}
