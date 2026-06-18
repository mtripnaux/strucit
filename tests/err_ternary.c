/* Doit echouer : l'operateur ternaire ?: n'existe pas dans la grammaire */
extern int printd(int i);

int main() {
    int a;
    int b;
    int m;
    a = 3;
    b = 7;
    m = (a > b) ? a : b;
    printd(m);
    return 0;
}
