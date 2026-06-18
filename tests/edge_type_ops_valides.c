/* Doit compiler : usages autorises par l'enonce sur les pointeurs */
extern int printd(int i);
extern void *malloc(int n);

int main() {
    int *p;
    int *q;
    int n;
    int diff;

    p = malloc(8);
    q = p + 1;     /* pointeur + entier -> pointeur : OK */
    q = p - 1;     /* pointeur - entier -> pointeur : OK */
    diff = q - p;  /* pointeur - pointeur -> entier : OK */
    n = *p;        /* '*' sur un pointeur : OK */
    p = &n;        /* '&' sur un int : OK */

    printd(diff);
    return 0;
}
