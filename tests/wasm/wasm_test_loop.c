int sum_to_n(int n) {
    int sum = 0;
    while (n > 0) {
        sum += n;
        n--;
    }
    return sum;
}

int main() {
    return sum_to_n(10);
}
