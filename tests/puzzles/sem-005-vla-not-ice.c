/* Should compile successfully in C mode and return 0.
   Semantic target: a is a VLA. */
int main(void) {
    const int n = 4;
    int a[n];
    return sizeof(a) == 4 * sizeof(int) ? 0 : 1;
}
