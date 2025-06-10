#include <vector>
#include <cstdlib>
#include <cstdio>

int main() {
    // Dimensions for matrix and vector
    int rows = 6;
    int cols = 6;

    float* mat = new float[rows * cols];
    float* vec = new float[cols];
    float* out = new float[rows];

    // Fill the matrix
    printf("Matrix:\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            mat[i * cols + j] = 1.0;
            printf("%.1f ", mat[i * cols + j]);
        }
        printf("\n");
    }

    // Initialize vector
    printf("\nVector:\n");
    for (int i = 0; i < cols; i++) {
        vec[i] = 2.0f;
        printf("%.1f ", vec[i]);
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

    printf("\n\nOutput Vector:\n");
    for (int i = 0; i < rows; i++) {
        printf("%.1f ", out[i]);
    }

    // Free allocated memory
    free(mat);
    free(vec);
    free(out);
}
