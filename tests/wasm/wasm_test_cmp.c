int compare_all(int a, int b) {
    int result = 0;
    if (a < b) result = 1;
    if (a > b) result = 2;
    if (a <= b) result = 3;
    if (a >= b) result = 4;
    if (a == b) result = 5;
    if (a != b) result = 6;
    return result;
}

int main() {
    return compare_all(5, 10);
}
