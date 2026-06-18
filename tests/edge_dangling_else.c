/* Stress: dangling else, if/if/else doit s'apparier au if le plus proche */
extern int printd(int i);

int main() {
    int a;
    int b;
    a = 1;
    b = 0;

    if (a == 1)
        if (b == 1)
            printd(1);
        else
            printd(2);     /* doit s'attacher au if(b==1), donc imprime 2 */

    if (a == 1)
        if (b == 0)
            printd(3);
    else
        printd(4);         /* meme regle : attache au if interne, donc pas atteint ici */

    return 0;
}
