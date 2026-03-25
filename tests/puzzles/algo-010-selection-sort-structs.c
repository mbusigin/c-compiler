/* Expected output:
1:amy 2:max 3:zoe
*/
#include <stdio.h>

struct Pair {
    int key;
    const char *name;
};

static void sort_pairs(struct Pair *a, int n) {
    int i, j;

    for (i = 0; i < n - 1; i++) {
        int min = i;
        for (j = i + 1; j < n; j++) {
            if (a[j].key < a[min].key) {
                min = j;
            }
        }
        if (min != i) {
            struct Pair tmp = a[i];
            a[i] = a[min];
            a[min] = tmp;
        }
    }
}

int main(void) {
    int i;
    struct Pair a[3] = {
        {3, "zoe"},
        {1, "amy"},
        {2, "max"}
    };

    sort_pairs(a, 3);
    for (i = 0; i < 3; i++) {
        printf("%d:%s%c", a[i].key, a[i].name, i == 2 ? '\n' : ' ');
    }
    return 0;
}
