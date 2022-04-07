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

	const int n = 1024;
	double* array = (double*) malloc(sizeof(double) * n);

	int i = 0;
	for(i = 0; i < n; ++i) {
		array[i] = i;
	}

	printf("Performing an OpenMP reduction...\n");
	double total = 0;

	#pragma omp parallel for reduction(+:total)
	for(i = 0; i < n; ++i) {
		total += array[i];
	}

	printf("Value of reduction is: %f\n", total);

}
