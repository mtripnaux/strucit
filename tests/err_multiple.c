/* Plusieurs erreurs syntaxiques distinctes dans le meme fichier : verifie
   que le parseur les detecte et les affiche TOUTES en une seule passe
   (panic-mode error recovery, dragon book 4.8.3) au lieu de s'arreter a
   la premiere. */
extern int printd(int i);

int && bad_global;

int main() {
    int a;
    a = 5 + ;
    printd(a);
@
    a = ) 3;
    printd(a);

    return 0;
}
