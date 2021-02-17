// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

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
