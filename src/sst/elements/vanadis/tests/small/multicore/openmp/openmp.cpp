// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

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
    omp_set_num_threads(3); // Set number of threads

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int cpu_id = thread_id; // Assign 1 threads to each core

        bind_thread_to_cpu(cpu_id);

        for (int i = 0; i < 3; i++) {
            if (thread_id == i) {
                printf("Thread %d bound to CPU %d\n", thread_id, sched_getcpu());
            }
            #pragma omp barrier
        }
    }

    return 0;
}
