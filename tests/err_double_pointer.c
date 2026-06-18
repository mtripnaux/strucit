/* Doit echouer : les pointeurs de pointeurs sont interdits (et la grammaire
   ne permet structurellement qu'une seule etoile en tete de declarateur) */
extern int printd(int i);

int main() {
    int a;
    int *p;
    int **pp;
    a = 5;
    p = &a;
    pp = &p;
    printd(**pp);
    return 0;
}
