#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#define NUM_THREADS 2

void thread_function(int core_id) {
    // Set thread affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }

    printf("Thread running on core %d\n", core_id);
    while (1); // Keep the thread running for demonstration purposes
}

int main() {
    omp_set_num_threads(NUM_THREADS); // Set the number of OpenMP threads

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num(); // Get the current thread's ID
        thread_function(thread_id);
    }

    return 0;
}
