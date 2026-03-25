/* Should compile successfully.
   Semantic target: f is array 5 of pointer to function(void) returning int. */
int (*f[5])(void);

int main(void) {
    return 0;
}
