/* Doit echouer (plusieurs erreurs attendues) : violations des regles de
   typage des operateurs (enonce 3.1, "Le type des expressions ..."). */
extern int printd(int i);
extern void *malloc(int n);

int main() {
    int x;
    int *p;
    int *q;
    int r;

    x = 5;
    p = malloc(4);
    q = malloc(4);

    r = p * q;   /* '*' interdit sur des pointeurs */
    r = p + q;   /* addition entre deux pointeurs interdite */
    r = x - p;   /* soustraction d'un pointeur a un entier interdite */
    r = -p;      /* '-' unaire ne s'applique qu'a un int */
    r = *x;      /* '*' unaire ne s'applique qu'a un pointeur */
    p = &p;      /* '&' ne s'applique qu'a un int ou une fonction */

    printd(r);
    return 0;
}
