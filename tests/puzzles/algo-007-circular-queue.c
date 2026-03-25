/* Expected output:
10 20 30 40
*/
#include <stdio.h>

struct Queue {
    int data[5];
    int head;
    int tail;
    int size;
};

static void enqueue(struct Queue *q, int value) {
    q->data[q->tail] = value;
    q->tail = (q->tail + 1) % 5;
    q->size++;
}

static int dequeue(struct Queue *q) {
    int value = q->data[q->head];
    q->head = (q->head + 1) % 5;
    q->size--;
    return value;
}

int main(void) {
    struct Queue q;

    q.head = 0;
    q.tail = 0;
    q.size = 0;
    enqueue(&q, 10);
    enqueue(&q, 20);
    enqueue(&q, 30);
    printf("%d ", dequeue(&q));
    enqueue(&q, 40);
    printf("%d %d %d\n", dequeue(&q), dequeue(&q), dequeue(&q));
    return 0;
}
