/* Doit echouer : sizeof ne s'applique qu'a une variable (cf enonce :
   "la taille en octets de la structure pointee par la variable passee en
   argument"), pas a un nom de type comme int ou void. */
extern int printd(int i);

int main() {
    printd(sizeof(int));
    return 0;
}
