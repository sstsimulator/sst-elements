#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mpi.h>
#include <omp.h>
#include <chrono>
#include <iostream>
#include "arielapi.h"

#define DEBUG 0
#define TIMING 1

int main(int argc, char* argv[]) {

    int prov;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &prov);

    if (argc < 2) {
        printf("Too few args\n");
        exit(1);
    }

    int len = atoi(argv[1]);

    if (len < 1) {
        printf("Please specify positive len\n");
        exit(1);
    }

    int rank = 0;
    int nranks = 0;

    int ret  = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (ret != MPI_SUCCESS) {
        printf("Error: MPI_Comm_rank retuned error: %d\n", ret);
        exit(1);
    }
    ret  = MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    if (ret != MPI_SUCCESS) {
        printf("Error: MPI_Comm_rank retuned error: %d\n", ret);
        exit(1);
    }

    if (len % nranks != 0) {
        printf("MPI ranks must divide vector length (len = %d, nranks = %d)\n", len, nranks);
        exit(1);
    }

    len = len / nranks;

    FILE *output = stdout;

    if (argc > 2) {
        int len = strlen(argv[2]) + 12;// Space for underscore, plus up to a 10 digit integer, plus the null character
        char *outfile = (char*)malloc(len);
        if (!outfile) {
            printf("Error allocating space for filename\n");
        }
        snprintf(outfile, len, "%s_%d", argv[2], rank);

        output = fopen(outfile, "w");
        if (!output) {
            printf("Unable to open %s\n", outfile);
            exit(1);
        }
    }


    int nthreads = omp_get_max_threads();

#if DEBUG
    printf("Running on %d ranks, %d threads per rank\n", nranks, nthreads);
#endif

    // Initialize
    int *vec = (int*) malloc(sizeof(int) * len);
    for (int i = 0; i < len; i++) {
        vec[i] = rank*len + i;
    }

    ariel_enable();

#if TIMING
    auto begin = std::chrono::high_resolution_clock::now();
#endif

    long int sum = 0;
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < len; i++) {
        sum += vec[i];
    }

#if TIMING
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    if (rank == 0) {
        std::cout << nranks << " " << nthreads << " " << duration/1000000 << "\n";
    }
#endif

    ariel_disable();

	long int tot = 0;
	MPI_Allreduce(
		&sum,
		&tot,
		1,
		MPI_LONG,
		MPI_SUM,
		MPI_COMM_WORLD);

    fprintf(output, "Rank %d partial sum is %ld, total sum is %d\n", rank, sum, tot);

    if (output != stdout) {
        int ret = fclose(output);
        if (ret) {
            perror("reduce.cc: Error closing output file");
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
#if DEBUG
        printf("Rank 0: Barrier complete.\n");
#endif
    }

    MPI_Finalize();
}
