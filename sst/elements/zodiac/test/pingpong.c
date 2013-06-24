#include <stdio.h>
#include "mpi.h"


int main(int argc, char* argv[]) {

	MPI_Init(&argc, &argv);
	int rank, npes;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

	double my_number = 0;
	int message_count = 10;
	int i;

	for(i = 0; i < message_count; i++) {
		if(rank % 2 == 0) {
			my_number = 5;
			MPI_Send(&my_number, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD);
		} else {
			my_number = 0;
			MPI_Status status;

			MPI_Recv(&my_number, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &status);
			printf("Recv'd a message from %d, message contents are: %f\n",
				rank - 1, my_number);
		}
	}

	MPI_Finalize();
}
