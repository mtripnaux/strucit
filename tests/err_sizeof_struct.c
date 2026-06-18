/* Doit echouer : sizeof(struct X) directement n'est pas prevu par la grammaire
   (seuls sizeof(expr), sizeof(int), sizeof(void) sont acceptes) */
extern int printd(int i);

struct trois {
    int a;
    int b;
    int c;
};

int main() {
    printd(sizeof(struct trois));
    return 0;
}
