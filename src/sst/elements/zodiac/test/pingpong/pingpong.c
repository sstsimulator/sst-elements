// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char* argv[]) {

	MPI_Init(&argc, &argv);

	int rank, npes;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

	double my_value = 5;

	int messages_per_size = 1000;
	//unsigned int message_


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
