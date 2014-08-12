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
