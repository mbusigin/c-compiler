/* Expected output:
7 5 1
*/
#include <stdio.h>

struct Stack {
    int data[8];
    int top;
};

static void push(struct Stack *s, int value) {
    s->data[s->top++] = value;
}

static int pop(struct Stack *s) {
    return s->data[--s->top];
}

int main(void) {
    struct Stack s;

    s.top = 0;
    push(&s, 1);
    push(&s, 5);
    push(&s, 7);
    printf("%d %d %d\n", pop(&s), pop(&s), pop(&s));
    return 0;
}
