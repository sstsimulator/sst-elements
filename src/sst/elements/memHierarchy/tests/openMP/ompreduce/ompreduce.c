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
