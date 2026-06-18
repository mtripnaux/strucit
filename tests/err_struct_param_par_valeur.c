/* Doit echouer : un parametre de fonction de type struct doit etre un pointeur */
extern int printd(int i);

struct point {
    int x;
    int y;
};

int f(struct point p) {
    return p.x;
}

int main() {
    printd(0);
    return 0;
}
