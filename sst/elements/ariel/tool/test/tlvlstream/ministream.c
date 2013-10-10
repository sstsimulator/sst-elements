#include <stdio.h>
#include <stdlib.h>
#include <tlvl.h>

int main(int argc, char* argv[]) {

	const int LENGTH = 2000;

	printf("Allocating arrays of size %d elements.\n", LENGTH);
	double* a = (double*) tlvl_malloc(sizeof(double) * LENGTH);
	double* b = (double*) tlvl_malloc(sizeof(double) * LENGTH);
	double* c = (double*) malloc(sizeof(double) * LENGTH);
	printf("Done allocating arrays.\n");

	int i;
	for(i = 0; i < LENGTH; ++i) {
		a[i] = i;
		b[i] = LENGTH - i;
		c[i] = 0;
	}

	for(i = 0; i < LENGTH; ++i) {
		c[i] = 2.0 * a[i] + 1.5 * b[i];
	}

	double sum = 0;
	for(i = 0; i < LENGTH; ++i) {
		sum += c[i];
	}

	printf("Sum of arrays is: %f\n", sum);
	printf("Freeing arrays...\n");

	tlvl_free(a);
	tlvl_free(b);
	free(c);

	printf("Done.\n");
}
