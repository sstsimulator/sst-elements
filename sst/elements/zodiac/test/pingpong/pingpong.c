#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char* argv[]) {

	MPI_Init(&argc, &argv);

	int rank, npes;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

	double my_value = 5;
	if(rank % 2 == 0) {
		MPI_Send(&my_value, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD);
	} else {
		my_value = 0;
		MPI_Request req;
		MPI_Irecv(&my_value, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &req);
		MPI_Status status;
		MPI_Wait(&req, &status);
		printf("Rank: %d recv'd a value: %f\n", rank, my_value);
	}

	MPI_Finalize();

}
