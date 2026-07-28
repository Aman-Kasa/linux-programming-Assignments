#include <stdio.h>
#include <stdlib.h>

// 1. Global Variable
int global_multiplier = 3;

// User-Defined Function 1: Decision-making
int is_multiple_of_two(int val) {
    if (val % 2 == 0) {
        return 1;
    } else {
        return 0;
    }
}

// User-Defined Function 2: Math/Processing
void apply_multiplier(int *val) {
    *val = (*val) * global_multiplier;
}

// User-Defined Function 3: Looping and standard library calls
void process_and_print(int *arr, int size) {
    printf("Processing Array:\n");
    for (int i = 0; i < size; i++) {
        apply_multiplier(&arr[i]);
        
        if (is_multiple_of_two(arr[i])) {
            printf("Value %d is Even\n", arr[i]);
        } else {
            printf("Value %d is Odd\n", arr[i]);
        }
    }
}

int main() {
    int count = 4; // Local variable on the stack

    // Dynamic memory allocation on the heap
    int *data = (int *)malloc(count * sizeof(int));
    if (data == NULL) {
        return 1; // Exit if allocation fails
    }

    // Initialize array data
    data[0] = 1; 
    data[1] = 2; 
    data[2] = 3; 
    data[3] = 4;

    // Call the processing function
    process_and_print(data, count);

    // Free dynamically allocated memory
    free(data);
    return 0;
}
