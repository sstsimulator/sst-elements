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
#include <stdlib.h>


int main(int argc, char* argv[]) {

	int thread_count = 0;

	#pragma omp parallel shared(thread_count)
	{
		#pragma omp critical
		{
			thread_count = thread_count + 1;
		}
	}

	printf("Counted: %d threads.\n", thread_count);
}
