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

#include <time.h>

#include <mpi.h>


int main(int argc, char* argv[]) {

	MPI_Init(&argc, &argv);

	int rank, npes;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

	if(rank == 0) {
		printf("PingPong (Round Trip) Message Benchmark for MPI\n");
	}

	//const n = 32768;
	const n = 8192;
	double* buffer = (double*) malloc(sizeof(double) * n);


	int msgsize;
	const int msgperitr = 1024;
	int msgcounter;
	MPI_Status status;

	if(rank == 0) {
		printf("%20s     %15s %30s\n", "Message Size (Bytes)", "Time (Secs)", "Bandwidth (B/s)");
	}

	for(msgsize = 1; msgsize <= n; msgsize += 128) {
		struct timespec start;
		clock_gettime(CLOCK_MONOTONIC, &start);

		for(msgcounter = 0; msgcounter < msgperitr; ++msgcounter) {
			if(rank == 0) {
				MPI_Send(buffer, msgsize, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
				MPI_Recv(buffer, msgsize, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD, &status);
			} else if(rank == 1) {
				MPI_Recv(buffer, msgsize, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
				MPI_Send(buffer, msgsize, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
			}
		}

		struct timespec end;
		clock_gettime(CLOCK_MONOTONIC, &end);

		if(rank == 0) {
			double time_sec = (((end.tv_sec + (end.tv_nsec * 1.0e-9)) -
				(start.tv_sec + (start.tv_nsec * 1.0e-9))) / (double) msgperitr);

			printf("%20d     %15.9f %30.6f\n", msgsize, time_sec,
				(((double) msgsize) / time_sec));
		}

		// So we get nice message increments
		if(msgsize == 1) {
			msgsize = 0;
		}
	}

	MPI_Finalize();

}
