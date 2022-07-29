// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>
#include <omp.h>


int main(int argc, char* argv[]) {

	printf("OpenMP API Test\n");

	int thread_count = omp_get_max_threads();

	printf("Maximum number of OpenMP threads is: %d\n", thread_count);

	int thread_count_now = omp_get_num_threads();

	printf("There are %d threads active now (should equal 1 since not in parallel region)\n", thread_count_now);

	#pragma omp parallel
	{
		#pragma omp single
		{
			thread_count_now = omp_get_num_threads();
			printf("There are now %d threads active (should equal OMP_NUM_THREADS since in parallel region)\n", thread_count_now);
		}
	}

}
