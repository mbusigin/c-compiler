int array_access(int *arr, int index) {
    return arr[index];
}

void array_set(int *arr, int index, int value) {
    arr[index] = value;
}

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    array_set(arr, 2, 10);
    return array_access(arr, 2);
}
