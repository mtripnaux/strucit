/* Deux structures mutuellement referencees par pointeur, A definie avant
   B mais utilisant deja "struct B *" dans son champ. Comme l'analyse
   semantique se termine entierement avant que codegen.c ne lise quoi que
   ce soit, B doit etre disponible a temps : ceci devrait COMPILER (pas un
   piege a proprement parler, ca confirme juste que l'ordre de definition
   des structs n'a pas besoin d'etre topologique). */
extern int printd(int i);
extern void *malloc(int n);

struct a {
    int v;
    struct b *suivant;
};

struct b {
    int v;
    struct a *suivant;
};

int main() {
    struct a *pa;
    struct b *pb;
    pa = malloc(8);
    pb = malloc(8);
    pa->v = 1;
    pa->suivant = pb;
    pb->v = 2;
    pb->suivant = pa;
    printd(pa->suivant->v);
    printd(pb->suivant->v);
    return 0;
}
