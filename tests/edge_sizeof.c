/* Stress: sizeof(variable), conforme a l'enonce (sizeof ne s'applique qu'a
   la variable passee en argument, pas a un nom de type) */
extern int printd(int i);
extern void *malloc(int n);

struct trois {
    int a;
    int b;
    int c;
};

int main() {
    struct trois *p;
    int x;
    p = malloc(sizeof(p));
    x = 7;

    printd(sizeof(p));       /* 12 (3 champs * 4) */
    printd(sizeof(p) + 1);   /* 13 */
    printd(sizeof(x));       /* 4 (pas une struct -> taille par defaut) */

    return 0;
}
