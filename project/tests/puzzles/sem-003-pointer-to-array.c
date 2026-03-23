/* Should compile successfully and return 0. */
int main(void) {
    int a[10];
    int (*p)[10] = &a;
    return sizeof(*p) == sizeof(a) ? 0 : 1;
}
