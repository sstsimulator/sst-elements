#include <stdio.h>
#include <stdlib.h>
#include <tlvl.h>

int main(int argc, char* argv[]) {

	const int LENGTH = 2000;

	printf("Allocating arrays of size %d elements.\n", LENGTH);
	double* a = (double*) tlvl_malloc(sizeof(double) * LENGTH);
	double* b = (double*) tlvl_malloc(sizeof(double) * LENGTH);
	double* fast_c = (double*) tlvl_malloc(sizeof(double) * LENGTH);

	printf("Allocation for fast_c is %llu\n", (unsigned long long int) fast_c);
	double* c = (double*) malloc(sizeof(double) * LENGTH);
	printf("Done allocating arrays.\n");

	int i;
	for(i = 0; i < LENGTH; ++i) {
		a[i] = i;
		b[i] = LENGTH - i;
		c[i] = 0;
	}

	// Issue a memory copy
	tlvl_memcpy(fast_c, c, sizeof(double) * LENGTH);

	printf("Perfoming the fast_c compute loop...\n");
	#pragma omp parallel for
	for(i = 0; i < LENGTH; ++i) {
		//printf("issuing a write to: %llu (fast_c)\n", ((unsigned long long int) &fast_c[i]));
		fast_c[i] = 2.0 * a[i] + 1.5 * b[i];
	}

	// Now copy results back
	tlvl_memcpy(c, fast_c, sizeof(double) * LENGTH);

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
