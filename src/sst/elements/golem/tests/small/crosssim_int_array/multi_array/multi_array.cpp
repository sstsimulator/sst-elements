#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <inttypes.h>

void fill_array(int32_t* arr, int32_t value, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        arr[i] = value;
    }
}

void print_matrix(int32_t* mat, uint32_t rows, uint32_t cols) {
    for (uint32_t i = 0; i < rows; i++) {
        for (uint32_t j = 0; j < cols; j++) {
            printf("%d ", mat[i * cols + j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_vector(int32_t* vec, uint32_t cols) {
    for (uint32_t i = 0; i < cols; i++) {
        printf("%d ", vec[i]);
    }
    printf("\n\n");
}

void setup_array(int32_t *mat, int32_t* vec, uint32_t tile_id) {
    int status_flag = 0;
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
}

void execute_mvm(uint32_t tile_id) {
    int status_flag = 0;
    asm volatile (
        "mvm %0, %1, x0"
      : "=r" (status_flag)
      : "r"(tile_id)
    );
}

void store_vector(int32_t* out, uint32_t tile_id) {
    int status_flag = 0;
    asm volatile (
        "mvm.s %0, %1, %2"
      : "=r" (status_flag)
      : "r"(out), "r"(tile_id)
      : "memory"
    );
}

void move_vector(uint32_t tile_id, uint32_t tile_id_new) {
    int status_flag = 0;
    asm volatile (
        "mvm.mv %0, %1, %2"
      : "=r" (status_flag)
      : "r"(tile_id), "r"(tile_id_new)
      : "memory"
    );
}

int main() {
    // Dimensions for matrix and vector
    int rows = 6;
    int cols = 6;
    int num_ping_pongs = 2;

    int32_t* matA = new int32_t[rows * cols];
    int32_t* vecA = new int32_t[cols];
    int32_t* outA = new int32_t[rows];

    int32_t* matB = new int32_t[rows * cols];
    int32_t* vecB = new int32_t[cols];
    int32_t* outB = new int32_t[rows];

    // Fill the matrices and vectors
    fill_array(matA, 3, rows * cols);
    fill_array(matB, 2, rows * cols);
    fill_array(vecA, 1, cols);
    fill_array(vecB, 4, cols);

    // Print matrices and vectors
    printf("Matrix A:\n");
    print_matrix(matA, rows, cols);

    printf("Matrix B:\n");
    print_matrix(matB, rows, cols);

    printf("Vector A:\n");
    print_vector(vecA, cols);

    printf("Vector B:\n");
    print_vector(vecB, cols);

    // Set up array
    setup_array(matA, vecA, 0);
    setup_array(matB, vecB, 1);

    // Run mvm
    execute_mvm(0);
    execute_mvm(1);

    for (int i = 0; i < num_ping_pongs; i++) {
        // Move vectors
        move_vector(1, 0);
        move_vector(0, 1);

        // Run mvm
        execute_mvm(0);
        execute_mvm(1);
    }

    // Store vector
    store_vector(outA, 0);
    store_vector(outB, 1);

    // Print output vector
    printf("Output Vector A:\n");
    print_vector(outA, cols);

    printf("Output Vector B:\n");
    print_vector(outB, cols);

    // Free allocated memory
    delete[] matA;
    delete[] vecA;
    delete[] outA;
    delete[] matB;
    delete[] vecB;
    delete[] outB;

    return 0;
}
