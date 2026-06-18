/* Stress: recursivite directe (factorielle, fibonacci) */
extern int printd(int i);

int fact(int n) {
    if (n <= 1)
        return 1;
    return n * fact(n - 1);
}

int fib(int n) {
    if (n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    printd(fact(5));   /* 120 */
    printd(fib(10));   /* 55 */
    return 0;
}
