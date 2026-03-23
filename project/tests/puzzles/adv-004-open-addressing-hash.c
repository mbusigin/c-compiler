/* Expected output:
33 1 0
*/
#include <stdio.h>

static int hash(int key) {
    return key % 7;
}

static void insert(int *table, const int *used, int key) {
    (void)used;
    {
        int i;
        int idx = hash(key);
        for (i = 0; i < 7; i++) {
            int pos = (idx + i) % 7;
            if (table[pos] == -1) {
                table[pos] = key;
                return;
            }
        }
    }
}

static int contains(const int *table, int key) {
    int i;
    int idx = hash(key);

    for (i = 0; i < 7; i++) {
        int pos = (idx + i) % 7;
        if (table[pos] == -1) {
            return 0;
        }
        if (table[pos] == key) {
            return 1;
        }
    }
    return 0;
}

int main(void) {
    int i;
    int dummy_used[7] = {0, 0, 0, 0, 0, 0, 0};
    int table[7];

    for (i = 0; i < 7; i++) {
        table[i] = -1;
    }
    insert(table, dummy_used, 10);
    insert(table, dummy_used, 17);
    insert(table, dummy_used, 24);
    insert(table, dummy_used, 33);
    printf("%d %d %d\n", table[1], contains(table, 33), contains(table, 18));
    return 0;
}
