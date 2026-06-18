/* Constante numerique qui depasse largement INT_MAX : atoi() n'a pas de
   detection de depassement en C, donc le comportement est indetermine,
   mais ca ne doit ni planter ni boucler. */
extern int printd(int i);

int main() {
    int a;
    a = 99999999999999999999999999999999999999;
    printd(a);
    return 0;
}
