/* Doit echouer : un champ de structure de type struct doit etre un pointeur */
extern int printd(int i);

struct interne {
    int v;
};

struct externe {
    struct interne in;   /* devrait etre "struct interne *in;" */
    int x;
};

int main() {
    printd(0);
    return 0;
}
