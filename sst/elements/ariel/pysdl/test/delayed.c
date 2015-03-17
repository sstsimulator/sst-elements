// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "ariel_client.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {

	printf("I am launching my main process...\n");

	const int N = 1024;
	double* my_array = (double*) malloc(sizeof(double) * N);
	int i;

	for(i = 0; i < N; ++i) {
		my_array[i] = (N + i) % 7;
	}

	printf("2nd value is %f\n", my_array[2]);
	printf("Enabling Ariel...\n");
	ariel_enable();

	printf("Now doing compute...\n");
	double sum = 0;
	for(i = 0; i < N; ++i) {
		sum += my_array[i];
	}

	printf("Sum is: %f\n", sum);
}
