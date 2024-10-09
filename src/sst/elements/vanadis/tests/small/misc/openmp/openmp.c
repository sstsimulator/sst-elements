// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

    setvbuf(stdout, NULL, _IONBF ,0);

    omp_lock_t lock;
    omp_init_lock(&lock);

    int nthreads, tid;
    char* tmp = getenv("OMP_NUM_THREADS");
    printf("OMP_NUM_THREADS %s\n", tmp ? tmp: "not set");

    /* Fork a team of threads giving them their own copies of variables */
    #pragma omp parallel private(nthreads, tid)
    {
        /* Obtain thread number */
        tid = omp_get_thread_num();

        /* Only master thread does this */
        if (tid == 0) 
        {
            nthreads = omp_get_num_threads();
            omp_set_lock( &lock );
            printf("Number of threads = %d\n", nthreads);
            omp_unset_lock( &lock );
        }

        omp_set_lock( &lock );
        printf("Hello World from thread = %d\n", tid);
        omp_unset_lock( &lock );

    } /* All threads join master thread and disband */

    printf("exit\n");
}
