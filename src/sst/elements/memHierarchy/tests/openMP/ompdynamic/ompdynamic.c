#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

	printf("OpenMP Dynamic Test Case\n");

	const int n = 1024;
	double* the_array = (double*) malloc(sizeof(double) * n);

	int i = 0;
	for(i = 0; i < n; ++i) {
		the_array[i] = i;
	}

	double sum;
	#pragma omp parallel for schedule(dynamic) reduction(+:sum)
	for(i = 0; i < n; ++i) {
		sum += the_array[i];
	}

	printf("Dynamic vector sum is: %f\n", sum);

}
