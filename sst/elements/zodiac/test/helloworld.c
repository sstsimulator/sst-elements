#include <stdio.h>
#include "mpi.h"

int main(int argc, char* argv[]) {

	MPI_Init(&argc, &argv);

	int rank, npes;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

	MPI_Barrier(MPI_COMM_WORLD);

	if(rank == 0) {
		printf("Hello World\n");
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
}
