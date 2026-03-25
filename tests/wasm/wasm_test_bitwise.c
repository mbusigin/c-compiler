int bitwise_ops(int a, int b) {
    int and_result = a & b;
    int or_result = a | b;
    int xor_result = a ^ b;
    int shl_result = a << 2;
    int shr_result = a >> 1;
    return and_result + or_result + xor_result + shl_result + shr_result;
}

int main() {
    return bitwise_ops(10, 5);
}
