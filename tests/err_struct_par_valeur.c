/* Doit echouer : une structure ne peut etre manipulee que par pointeur
   (enonce 3.1, contrainte semantique explicite) */
extern int printd(int i);

struct point {
    int x;
    int y;
};

int main() {
    struct point p;
    printd(0);
    return 0;
}
