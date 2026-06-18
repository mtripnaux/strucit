/* Stress: && || comme valeurs (pas seulement en condition de if/while),
   combinaisons imbriquees, court-circuit */
extern int printd(int i);

int main() {
    int a;
    int b;
    int c;
    int r;
    a = 5;
    b = 10;
    c = 15;

    r = (a < b) && (b < c);
    printd(r);                       /* 1 */

    r = (a > b) || (b < c);
    printd(r);                       /* 1 */

    r = (a > b) && (b < c);
    printd(r);                       /* 0 */

    r = (a < b) && (b < c) && (a != b);
    printd(r);                       /* 1 */

    r = (a == b) || (b == c) || (a < c);
    printd(r);                       /* 1 */

    if ((a < b) && (c > b))
        printd(100);
    else
        printd(200);

    while ((a < c) && (a != b)) {
        a = a + 1;
    }
    printd(a);                       /* 10 */

    return 0;
}
