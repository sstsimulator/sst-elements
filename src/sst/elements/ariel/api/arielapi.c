// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "arielapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#ifdef ENABLE_ARIEL_MPI
#include <mpi.h>
#endif

/* These definitions are replaced during simulation */

void ariel_enable() {
    printf("ARIEL: ENABLE called in Ariel API.\n");
}

void ariel_disable() {
    printf("ARIEL: DISABLE called in Ariel API.\n");
}

void ariel_fence() {
    printf("ARIEL: FENCE called in Ariel API.\n");
}

uint64_t ariel_cycles() {
    return 0;
}

void ariel_output_stats() {
    printf("ARIEL: Request to print statistics.\n");
}

void ariel_malloc_flag(int64_t id, int count, int level) {
    printf("ARIEL: flagging next %d mallocs at id %" PRId64 "\n", count, id);
}

// To ensure that the Pintool (fesimple.cc) numbers our application's OpenMP threads
// from 0..N-1, we need to run an OpenMP parallel region before calling MPI Init.
// Otherwise, some MPI threads which aren't used for our application will be
// numbered 1 and 2.
void omp_parallel_region() {
    volatile int x = 0;
#if defined(_OPENMP)
#pragma omp parallel
    {
#pragma omp critical
        {
            x += 1;
        }
    }
#else
    printf("ERROR: arielapi.c: libarielapi was compiled without OpenMP enabled\n");
    exit(1);
#endif
}

// This function only exists to get mapped by the frontend. It should only be called
// from MPI_Init or MPI_Init_thread to allow the frontend to distinguish between our
// custom versions of of those functions and the normal MPI library's versions.
int _api_mpi_init() {
    printf("notifying fesimple\n");
}

// Custom version of MPI_Init. We override the normal version in order to call an
// OpenMP parallel region to ensure threads are numbered properly by the frontend.
int MPI_Init(int *argc, char ***argv) {
#ifdef ENABLE_ARIEL_MPI
    // Communicate to the frontend that we have replaced the nomal MPI_Init with
    // the one in the Ariel API
    _api_mpi_init();
    omp_parallel_region();
    return PMPI_Init(argc, argv);
#else
    printf("Error: arielapi.c: MPI_Init called in arielapi.c but this file was compiled without MPI.\n");
    exit(1);
#endif
}

// Custom version of MPI_Init_thread. We override the normal verison in order to call an
// OpenMP parallel region to ensure threads are numbered properly by the frontend.
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) {
#ifdef ENABLE_ARIEL_MPI
    // Communicate to the frontend that we have replaced the nomal MPI_Init_thread with
    // the one in the Ariel API
    _api_mpi_init();
    omp_parallel_region();
    return PMPI_Init_thread(argc, argv, required, provided);
#else
    printf("Error: arielapi.c: MPI_Init_thread called in arielapi.c but this file was compiled without MPI. Please recompile the API with `CC=mpicc make`123123.\n");
    exit(1);
#endif
}
