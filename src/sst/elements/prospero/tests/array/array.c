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

int main(int argc, char* argv[]) {
	printf("Running arrays...\n");

	const int SIZE = 131072;
	int i;
	double sum = 0;
	double* the_array = (double*) malloc(sizeof(double) * SIZE);

	for(i = 0; i < SIZE; ++i) {
		the_array[i] = ((double) i) * 2.5;
	}

	for(i = 0; i < SIZE; ++i) {
		sum += the_array[i];
	}

	printf("Total is %10.5f\n", sum);

	free(the_array);
}
