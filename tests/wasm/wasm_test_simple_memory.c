int simple_memory_test() {
    int x = 42;
    int *p = &x;
    return *p;
}

int main() {
    return simple_memory_test();
}