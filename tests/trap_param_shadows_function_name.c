/* Le parametre d'une fonction porte le meme nom que la fonction elle-meme.
   enregistrer_fonction exclut explicitement le nom de la fonction lors de
   la collecte des parametres "complexes" (cf strcmp(cur->id, nom) != 0
   dans l'ancienne version) : verifie que ce genre de heuristique ne casse
   pas un cas legitime ou le nom du parametre coincide juste par hasard
   avec celui de la fonction. */
extern int printd(int i);

int valeur(int valeur) {
    return valeur * 2;
}

int main() {
    printd(valeur(21));
    return 0;
}
