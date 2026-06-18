/* Erreur syntaxique juste avant la fin du fichier : aucun ';' apres elle
   pour resynchroniser. Le parseur doit echouer proprement (pas de crash,
   pas de boucle infinie), meme si la recuperation panic-mode ne trouve
   jamais de point de resynchronisation. */
extern int printd(int i);

int main() {
    int a;
    a = 5 +
