#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <inttypes.h>

int main() {
    int rows = 6, cols = 6;
    int32_t* mat = new int32_t[rows * cols];
    int32_t* vec = new int32_t[cols];
    int32_t* out = new int32_t[rows];

    // Fill the matrix and print it
    printf("Matrix:\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            mat[i * cols + j] = 1;
            printf("%d ", mat[i * cols + j]);
        }
        printf("\n");
    }

    // Initialize vector and print it
    printf("\nVector:\n");
    for (int i = 0; i < cols; i++) {
        vec[i] = 2;
        printf("%d ", vec[i]);
    }

    int status_flag;
    int tile_id = 0;

    // Perform MVM
    asm volatile (
        "mvm.set %0, %1, %2"
      : "=r" (status_flag)
      : "r"(mat), "r"(tile_id)
      : "memory"
    );

    asm volatile (
        "mvm.l %0, %1, %2"
      : "=r" (status_flag)
      : "r"(vec), "r"(tile_id)
      : "memory"
    );

    asm volatile (
        "mvm %0, %1, x0"
      : "=r" (status_flag)
      : "r"(tile_id)
    );

    asm volatile (
        "mvm.s %0, %1, %2"
      : "=r" (status_flag)
      : "r"(out), "r"(tile_id)
      : "memory"
    );

    // Print output vector
    printf("\n\nOutput Vector:\n");
    for (int i = 0; i < rows; i++) {
        printf("%d ", out[i]);
    }

    // Free allocated memory
    delete[] mat;
    delete[] vec;
    delete[] out;

    return 0;
}
