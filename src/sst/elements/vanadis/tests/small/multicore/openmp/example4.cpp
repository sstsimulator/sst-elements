#include <omp.h>
#include <stdio.h>
#include <sched.h>

#define MATRIX_SIZE 3
#define NUM_THREADS 2

void matrix_vector_multiply(float matrix[MATRIX_SIZE][MATRIX_SIZE], float vector[MATRIX_SIZE], float result[MATRIX_SIZE]) {
    for (int i = 0; i < MATRIX_SIZE; i++) {
        result[i] = 0;
        for (int j = 0; j < MATRIX_SIZE; j++) {
            result[i] += matrix[i][j] * vector[j];
        }
    }
}

int main() {
    omp_set_num_threads(NUM_THREADS);

    // Define matrices
    float matrix0[MATRIX_SIZE][MATRIX_SIZE] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };

    float matrix1[MATRIX_SIZE][MATRIX_SIZE] = {
        {9, 8, 7},
        {6, 5, 4},
        {3, 2, 1}
    };

    // Define vectors
    float vector0[MATRIX_SIZE] = {1, 2, 3};
    float vector1[MATRIX_SIZE] = {3, 2, 1};

    // Result arrays
    float result0[MATRIX_SIZE];
    float result1[MATRIX_SIZE];

    // Ping-pong processing
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();

        for (int round = 0; round < 5; round++) { // Ping-pong alternates 5 rounds
            if (thread_id == 0) {
                int core_id = sched_getcpu(); // Get the core ID the thread is running on
                printf("Thread 0 running on core %d processing vector0 with matrix0 (Round %d)\n", core_id, round);

                matrix_vector_multiply(matrix0, vector0, result0);

                printf("Thread 0 result: ");
                for (int i = 0; i < MATRIX_SIZE; i++) {
                    printf("%f ", result0[i]);
                }
                printf("\n");

                // Simulate synchronization point
                #pragma omp barrier
            } else if (thread_id == 1) {
                int core_id = sched_getcpu(); // Get the core ID the thread is running on
                printf("Thread 1 running on core %d processing vector1 with matrix1 (Round %d)\n", core_id, round);

                matrix_vector_multiply(matrix1, vector1, result1);

                printf("Thread 1 result: ");
                for (int i = 0; i < MATRIX_SIZE; i++) {
                    printf("%f ", result1[i]);
                }
                printf("\n");

                // Simulate synchronization point
                #pragma omp barrier
            }
        }
    }

    return 0;
}
