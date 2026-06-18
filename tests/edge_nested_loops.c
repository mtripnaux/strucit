/* Stress: boucles imbriquees (for/while/for) pour verifier l'unicite des labels */
extern int printd(int i);

int main() {
    int i;
    int j;
    int k;
    int total;
    total = 0;

    for (i = 0; i < 3; i = i + 1) {
        j = 0;
        while (j < 3) {
            for (k = 0; k < 3; k = k + 1) {
                if (k == j)
                    total = total + 1;
                else
                    total = total + 0;
            }
            j = j + 1;
        }
    }

    printd(total);   /* 3*3 = 9 (une correspondance k==j par tour de j, pour chaque i) */
    return 0;
}
