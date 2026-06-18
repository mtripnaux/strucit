/* Stress: struct a plusieurs champs, deux structs qui se referencent,
   acces chaine via plusieurs ->  */
extern int printd(int i);
extern void *malloc(int n);

struct noeud {
    int valeur;
    struct noeud *gauche;
    struct noeud *droite;
};

int profondeur_gauche(struct noeud *n) {
    if (n == 0)
        return 0;
    return 1 + profondeur_gauche(n->gauche);
}

int main() {
    struct noeud *racine;
    struct noeud *enfant;
    struct noeud *petit_enfant;

    racine = malloc(sizeof(racine));
    enfant = malloc(sizeof(enfant));
    petit_enfant = malloc(sizeof(petit_enfant));

    racine->valeur = 1;
    racine->gauche = enfant;
    racine->droite = 0;

    enfant->valeur = 2;
    enfant->gauche = petit_enfant;
    enfant->droite = 0;

    petit_enfant->valeur = 3;
    petit_enfant->gauche = 0;
    petit_enfant->droite = 0;

    printd(racine->valeur);                  /* 1 */
    printd(racine->gauche->valeur);           /* 2 */
    printd(racine->gauche->gauche->valeur);   /* 3 */
    printd(profondeur_gauche(racine));        /* 3 */

    return 0;
}
