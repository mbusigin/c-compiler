/* Should compile successfully.
   Semantic target: x has type int * const. */
typedef int *P;
const P x = 0;

int main(void) {
    return 0;
}
