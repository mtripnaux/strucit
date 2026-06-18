/* Stress: priorites d'operateurs, parentheses, unaire moins, melange */
extern int printd(int i);

int main() {
    int a;
    int b;
    int c;
    int d;
    a = 2;
    b = 3;
    c = 4;
    d = 5;

    printd(a + b * c);              /* 2 + 12 = 14 */
    printd((a + b) * c);            /* 5 * 4 = 20 */
    printd(a - b - c);              /* (2-3)-4 = -5, gauche-vers-droite */
    printd(a - (b - c));             /* 2-(-1) = 3 */
    printd(-a + b);                 /* -2+3 = 1 */
    printd(-(a + b));                /* -5 */
    printd(a * b + c * d);          /* 6+20=26 */
    printd(a * (b + c) * d);        /* 2*7*5=70 */
    printd(d / b);                  /* 5/3 = 1 */
    printd(d / b * c);              /* (5/3)*4 = 4 */
    printd(((a + b) - (c - d)) * (a + 1)); /* ((5)-(-1))*3 = 18 */
    return 0;
}
