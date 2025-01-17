#include <omp.h>
#include <stdio.h>
#include <sched.h>

int main() {
    // Get the number of available processors
    int num_procs = 2;

    // Set the number of threads to the number of processors
    omp_set_num_threads(num_procs);

    // Parallel region
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int core_id = sched_getcpu(); // Get the core ID the thread is running on
        int num_threads = omp_get_num_threads();

        // Print thread and CPU binding information
        printf("Thread %d of %d is running on processor %d\n", thread_id, num_threads, core_id);
    }

    return 0;
}
