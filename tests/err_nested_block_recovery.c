/* Erreur syntaxique profondement imbriquee (if dans while dans if) :
   verifie que la recuperation au niveau expression_statement fonctionne
   peu importe la profondeur d'imbrication. */
extern int printd(int i);

int main() {
    int a;
    a = 0;
    if (a == 0) {
        while (a < 3) {
            if (a == 1) {
                a = ;            /* erreur ici, profondement imbriquee */
            }
            a = a + 1;
            printd(a);
        }
    }
    return 0;
}
