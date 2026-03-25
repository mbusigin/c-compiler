/* Expected output:
2 3 4 5
*/
#include <stdio.h>

struct Queue {
    int data[4];
    int head;
    int tail;
    int size;
};

static void push(struct Queue *q, int x) {
    q->data[q->tail] = x;
    q->tail = (q->tail + 1) % 4;
    q->size++;
}

static int pop(struct Queue *q) {
    int x = q->data[q->head];
    q->head = (q->head + 1) % 4;
    q->size--;
    return x;
}

int main(void) {
    struct Queue q = {{0, 0, 0, 0}, 0, 0, 0};

    push(&q, 1);
    push(&q, 2);
    push(&q, 3);
    pop(&q);
    push(&q, 4);
    push(&q, 5);
    printf("%d %d %d %d\n", pop(&q), pop(&q), pop(&q), pop(&q));
    return 0;
}
