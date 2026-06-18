/* Piege : "break" n'est pas un mot reserve dans ANSI-C.l, il est donc lexe
   comme un IDENTIFIER ordinaire. "break;" est alors une expression_statement
   valide syntaxiquement (reference a un identifiant), et n'est PAS rejete par
   l'analyse semantique (qui ne verifie pas les identifiants simples). On
   s'attend a ce que ceci "compile" en produisant un code backend invalide
   (reference a une variable "break" jamais declaree). */
extern int printd(int i);

int main() {
    int i;
    i = 0;
    while (i < 5) {
        if (i == 3)
            break;
        printd(i);
        i = i + 1;
    }
    return 0;
}
