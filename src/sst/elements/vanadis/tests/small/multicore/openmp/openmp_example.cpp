#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

// Function to set CPU affinity
void bind_thread_to_cpu(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);      // Clear the set
    CPU_SET(cpu_id, &cpuset); // Add the specified CPU to the set

    pid_t thread_id = gettid(); // Get the thread ID
    if (sched_setaffinity(thread_id, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("sched_setaffinity failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    omp_set_num_threads(3); // Set number of threads (2 threads per core * 2 cores)

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int cpu_id = thread_id; // Assign 2 threads to each core

        bind_thread_to_cpu(cpu_id);

        printf("Thread %d bound to CPU %d\n", thread_id, sched_getcpu());
    }

    return 0;
}
