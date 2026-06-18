/* Stress: pointeurs de fonctions passes en parametre et appeles */
extern int printd(int i);

int carre(int x) { return x * x; }
int cube(int x) { return x * x * x; }

int applique(int (*f)(int n), int v) {
    return (*f)(v);
}

int main() {
    int (*op)(int n);
    op = &carre;
    printd((*op)(4));        /* 16 */
    op = &cube;
    printd((*op)(3));        /* 27 */
    printd(applique(&carre, 5));  /* 25 */
    printd(applique(&cube, 2));   /* 8 */
    return 0;
}
