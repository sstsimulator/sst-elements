#include <omp.h>
#include <stdio.h>
#include <sched.h>

#define NUM_THREADS 2
#define WORK_ITERATIONS 5

void thread_function(int thread_id) {
    int core_id = sched_getcpu(); // Get the core ID the thread is running on
    printf("Thread %d is running on core %d\n", thread_id, core_id);

    for (int i = 0; i < WORK_ITERATIONS; i++) {
        printf("Thread %d on core %d: Work iteration %d\n", thread_id, core_id, i);
    }

    printf("Thread %d on core %d has finished its work\n", thread_id, core_id);
}

int main() {
    omp_set_num_threads(NUM_THREADS); // Set the number of OpenMP threads

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num(); // Get the thread ID
        thread_function(thread_id); // Call the thread function
    }

    return 0;
}
