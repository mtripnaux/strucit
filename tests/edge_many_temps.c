/* Stress: expression tres profonde pour forcer beaucoup de temporaires
   et tester la reutilisation (Sethi-Ullman) */
extern int printd(int i);

int main() {
    int a; int b; int c; int d; int e; int f; int g; int h;
    a = 1; b = 2; c = 3; d = 4; e = 5; f = 6; g = 7; h = 8;

    printd(((a + b) * (c - d)) / ((e + f) * (g - h)) +
           ((a * b) - (c / d)) * ((e - f) + (g * h)) -
           (((a + b + c) * (d - e - f)) / (g + h + 1)));

    return 0;
}
