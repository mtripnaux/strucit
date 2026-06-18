/* sizeof() avec rien dedans : ne correspond a aucune des 3 formes
   acceptees (sizeof unary_expression / sizeof(int) / sizeof(void)).
   Doit etre une erreur syntaxique propre, pas un crash du parseur. */
extern int printd(int i);

int main() {
    int x;
    x = sizeof();
    printd(x);
    return 0;
}
