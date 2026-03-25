/* Expected diagnostic: arrays are not assignable. */
int main(void) {
    int a[3] = {1, 2, 3};
    int b[3] = {4, 5, 6};
    a = b;
    return 0;
}
