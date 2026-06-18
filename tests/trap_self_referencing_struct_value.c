/* Une structure qui se contient elle-meme PAR VALEUR (pas par pointeur) :
   taille infinie en C standard, et de toute facon interdit par l'enonce
   (structs uniquement par pointeur). Doit etre rejete par
   verifier_struct_par_pointeur, pas planter en boucle infinie pendant le
   calcul de taille/offset. */
struct noeud {
    int valeur;
    struct noeud suivant;   /* devrait etre "struct noeud *suivant;" */
};

int main() {
    return 0;
}
