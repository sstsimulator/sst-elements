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
#include <mpi.h>

int main(int argc, char* argv[]) {

	MPI_Init(&argc, &argv);

	int rank = 0;
	int npes = 0;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

	if(rank == 0) {
		printf("SST Sirius Allreduce Test (Ranks: %d)\n", npes);
	}

	double my_value = 5.0;
	double total = 0;

	int bench_count = 1;
	int counter;

	if(argc > 1) {
		bench_count = atoi(argv[1]);
	}

	if(rank == 0) {
		printf("Performing: %d all reductions.\n", bench_count);
	}

	for(counter = 0; counter < bench_count; ++counter) {
		MPI_Allreduce(&my_value, &total, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	}

	if(rank == 0) {
		printf("Value should be: %f\n", (my_value * npes));
		printf("Value is: %f\n", total);
	}

	MPI_Finalize();

	return 0;
}
