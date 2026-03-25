/* Expected output:
2 3 1 0
*/
#include <stdio.h>

struct Entry {
    int key;
    int count;
};

static int find_or_add(struct Entry *entries, int *n, int key) {
    int i;

    for (i = 0; i < *n; i++) {
        if (entries[i].key == key) {
            return i;
        }
    }
    entries[*n].key = key;
    entries[*n].count = 0;
    (*n)++;
    return *n - 1;
}

static int get_count(const struct Entry *entries, int n, int key) {
    int i;

    for (i = 0; i < n; i++) {
        if (entries[i].key == key) {
            return entries[i].count;
        }
    }
    return 0;
}

int main(void) {
    int i;
    int n = 0;
    int data[6] = {4, 2, 4, 3, 2, 2};
    struct Entry entries[6];

    for (i = 0; i < 6; i++) {
        int idx = find_or_add(entries, &n, data[i]);
        entries[idx].count++;
    }
    printf("%d %d %d %d\n",
           get_count(entries, n, 4),
           get_count(entries, n, 2),
           get_count(entries, n, 3),
           get_count(entries, n, 9));
    return 0;
}
