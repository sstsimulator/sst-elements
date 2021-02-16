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
#include <stdlib.h>
#include "ariel.h"

int main(int argc, char* argv[]) {

	const int LENGTH = 2000;

	printf("Allocating arrays of size %d elements.\n", LENGTH);
        ariel_malloc_flag(0, 1, 1);
	double* a = (double*) malloc(sizeof(double) * LENGTH);
        ariel_malloc_flag(1, 1, 1);
	double* b = (double*) malloc(sizeof(double) * LENGTH);
        ariel_malloc_flag(2, 1, 1);
	double* c = (double*) malloc(sizeof(double) * LENGTH);
	printf("Done allocating arrays.\n");

	int i;
	for(i = 0; i < LENGTH; ++i) {
		a[i] = i;
		b[i] = LENGTH - i;
		c[i] = 0;
	}

	printf("Perfoming the fast_c compute loop...\n");
	#pragma omp parallel for
	for(i = 0; i < LENGTH; ++i) {
		//printf("issuing a write to: %llu (fast_c)\n", ((unsigned long long int) &fast_c[i]));
		c[i] = 2.0 * a[i] + 1.5 * b[i];
	}

	double sum = 0;
	for(i = 0; i < LENGTH; ++i) {
		sum += c[i];
	}

	printf("Sum of arrays is: %f\n", sum);
	printf("Freeing arrays...\n");

	free(a);
	free(b);
	free(c);

	printf("Done.\n");
}
