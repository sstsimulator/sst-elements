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

	const int n = 20;
	int** the_array = (int**) malloc(sizeof(int*) * n);

	int i = 0;
	for(i = 0; i < n; ++i) {
		the_array[i] = (int*) malloc(sizeof(int) * n);
	}

	#pragma omp parallel for
	for(i = 0; i < n; ++i) {
		int j = 0;

		for(j = 0; j < n; ++j) {
			if(j < i) {
				the_array[i][j] = 1;
			} else {
				the_array[i][j] = 0;
			}
		}
	}

	printf("The matrix is:\n\n");

	for(i = 0; i < n; ++i) {
		int j = 0;

		for(j = 0; j < n; ++j) {
			printf("%d ", the_array[i][j]);
		}

		printf("\n");
	}

}
