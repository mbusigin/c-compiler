int add(int a, int b) {
    return a + b;
}

int call_add() {
    return add(3, 5);
}

int main() {
    return call_add();
}