/* Doit echouer : le type de retour struct doit etre un pointeur sur structure */
extern int printd(int i);
extern void *malloc(int n);

struct point {
    int x;
    int y;
};

struct point creer() {
    struct point *p;
    p = malloc(8);
    return *p;
}

int main() {
    printd(0);
    return 0;
}
