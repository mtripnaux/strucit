/* "struct Inconnue" n'est jamais definie nulle part dans le fichier.
   Rien dans semantic.c ne verifie qu'un type "struct X" reference existe
   reellement : ca devrait sans doute etre une erreur, mais au minimum ca
   ne doit pas planter (offset/taille par defaut a 4 quand la struct n'est
   pas trouvee, cf obtenir_taille_struct/obtenir_offset_champ). */
extern int printd(int i);
extern void *malloc(int n);

struct inconnue *p;

int main() {
    p = malloc(sizeof(p));
    printd(0);
    return 0;
}
