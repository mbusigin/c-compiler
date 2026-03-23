/* Should compile successfully.
   Semantic target:
   - f: function returning int *
   - g: pointer to function returning int */
int *f(void);
int (*g)(void);

int main(void) {
    return 0;
}
