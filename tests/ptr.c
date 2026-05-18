extern int printd(int i);
extern void *malloc(int n);

int main() {
    int *p;
    int *q;

    p = malloc(4);
    q = malloc(4);

    *p = 10;
    *q = 20;
    printd(*p);
    printd(*q);

    *p = *p + *q;
    printd(*p);

    return 0;
}
