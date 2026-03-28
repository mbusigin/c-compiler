/* Test case for array subscript assignment */
#include <stdio.h>

int main() {
    int arr[10];
    
    // Test array subscript assignment
    arr[5] = 42;
    
    // Check result
    if (arr[5] == 42) {
        printf("PASS: arr[5] = %d\n", arr[5]);
        return 0;
    } else {
        printf("FAIL: arr[5] = %d (expected 42)\n", arr[5]);
        return 1;
    }
}
