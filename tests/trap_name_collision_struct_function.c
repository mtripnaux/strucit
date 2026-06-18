/* Une structure et une fonction qui portent le meme nom. La table des
   symboles cherche par ->id sans distinguer le ->type : si la recherche
   tombe sur le mauvais symbole (struct au lieu de fonction, ou
   l'inverse), ca peut produire un comportement incoherent sans jamais
   signaler d'erreur claire. */
extern int printd(int i);
extern void *malloc(int n);

struct point {
    int x;
    int y;
};

int point(int v) {
    return v + 1;
}

int main() {
    struct point *p;
    p = malloc(8);
    p->x = 1;
    printd(point(p->x));
    return 0;
}
