/* Stress: fonctions void avec et sans "return;" explicite, return premature */
extern int printd(int i);

void log_positif(int x) {
    if (x < 0)
        return;
    printd(x);
}

void rien() {
}

int main() {
    log_positif(5);
    log_positif(-3);   /* ne doit rien afficher */
    rien();
    return 0;
}
