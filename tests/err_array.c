/* Doit echouer : les tableaux [] n'existent pas dans la grammaire */
extern int printd(int i);

int main() {
    int tab[10];
    tab[0] = 1;
    printd(tab[0]);
    return 0;
}
