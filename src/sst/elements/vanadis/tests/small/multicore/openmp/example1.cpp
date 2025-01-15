#include <stdio.h>
#include <omp.h>
int main() {
    // Set the number of threads dynamically
    omp_set_num_threads(2);
    // Declare and initialize an array
    int array[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int size = sizeof(array) / sizeof(array[0]);
    int sum = 0;
    // Parallel region with dynamic scheduling and reduction
    #pragma omp parallel for schedule(dynamic) shared(array, size) reduction(+:sum)
    for (int i = 0; i < size; i++) {
        int thread_id = omp_get_thread_num();
        int num_threads = omp_get_num_threads();
        // Perform operation on each array element
        array[i] *= 2;
        // Print processing details
        printf("Thread %d out of %d threads processed element %d, new value: %d\n",
               thread_id, num_threads, i, array[i]);
        // Add the modified value to the sum
        sum += array[i];
    }
    // Output the final sum
    printf("The sum of the modified array elements is: %d\n", sum);
    return 0;
}
