#include <cstdio>
#include <cstdlib>
#include <mpi.h>
#include <omp.h>
#include <chrono>
#include <iostream>
#include "arielapi.h"

#define DEBUG 0

int main(int argc, char* argv[]) {

    MPI_Init(&argc, &argv);

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
    auto begin = std::chrono::high_resolution_clock::now();
    long int sum = 0;
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < len; i++) {
        sum += vec[i];
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    if (rank == 0) {
        std::cout << nranks << " " << nthreads << " " << duration/1000000 << "\n";
    }
    ariel_disable();

#if DEBUG
    printf("Rank %d: sum is %ld\n", rank, sum);
#endif

	long int tot = 0;
	MPI_Allreduce(
		&sum,
		&tot,
		1,
		MPI_LONG,
		MPI_SUM,
		MPI_COMM_WORLD);


#if DEBUG
    printf("Rank %d: tot is %ld\n", rank, tot);
#endif


    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
#if DEBUG
        printf("Rank 0: Barrier complete.\n");
#endif
    }

    MPI_Finalize();
}
