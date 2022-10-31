
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef USE_MKL
#include "mkl.h"
#endif

#ifdef USE_CBLAS
#include "cblas.h"
#endif

#ifdef USE_ESSL
#include "essl.h"
#endif

//#define DGEMM_RESTRICT __restrict__
#define DGEMM_RESTRICT 

// ------------------------------------------------------- //
// Function: get_seconds
//
// Vendor may modify this call to provide higher resolution
// timing if required
// ------------------------------------------------------- //
double get_seconds() {
	struct timeval now;
	gettimeofday(&now, NULL);

	const double seconds = (double) now.tv_sec;
	const double usec    = (double) now.tv_usec;

	return seconds + (usec * 1.0e-6);
}

// ------------------------------------------------------- //
// Function: main
//
// Modify only in permitted regions (see comments in the
// function)
// ------------------------------------------------------- //
int main(int argc, char* argv[]) {

	// ------------------------------------------------------- //
	// DO NOT CHANGE CODE BELOW
	// ------------------------------------------------------- //

	//int N = 256;
	//int repeats = 8;
	int N = SIZE;
	int repeats = REPEATS;

    	double alpha = 1.0;
    	double beta  = 1.0;

	if(argc > 1) {
		N = atoi(argv[1]);
		printf("Matrix size input by command line: %d\n", N);

		if(argc > 2) {
			repeats = atoi(argv[2]);

			if(repeats < 4) {
				fprintf(stderr, "Error: repeats must be at least 4, setting is: %d\n", repeats);
				exit(-1);
			}

			printf("Repeat multiply %d times.\n", repeats);

            if(argc > 3) {
                alpha = (double) atof(argv[3]);

                if(argc > 4) {
                    beta = (double) atof(argv[4]);
                }
            }
		} else {
			printf("Repeat multiply defaulted to %d\n", repeats);
		}
	} else {
		printf("Matrix size defaulted to %d\n", N);
	}

    	printf("Alpha =    %f\n", alpha);
    	printf("Beta  =    %f\n", beta);
    	printf("Iterations =    %d\n", repeats);

#if 0
	if(N < 128) {
		printf("Error: N (%d) is less than 128, the matrix is too small.\n", N);
		exit(-1);
	}
#endif

	printf("Allocating Matrices...\n");

#if 0
	double* DGEMM_RESTRICT matrixA = (double*) malloc(sizeof(double) * N * N);
	double* DGEMM_RESTRICT matrixB = (double*) malloc(sizeof(double) * N * N);
	double* DGEMM_RESTRICT matrixC = (double*) malloc(sizeof(double) * N * N);
#endif
	double DGEMM_RESTRICT matrixA[sizeof(double) * N * N];
	double DGEMM_RESTRICT matrixB[sizeof(double) * N * N];
	double DGEMM_RESTRICT matrixC[sizeof(double) * N * N];


	printf("Allocation complete, populating with values...\n");

	int i, j, k, r;

	#pragma omp parallel for
	for(i = 0; i < N; i++) {
		for(j = 0; j < N; j++) {
			matrixA[i*N + j] = 2.0;
			matrixB[i*N + j] = 0.5;
			matrixC[i*N + j] = 1.0;
		}
	}

	printf("Performing multiplication...\n");

	const double start = get_seconds();

	// ------------------------------------------------------- //
	// VENDOR NOTIFICATION: START MODIFIABLE REGION
	//
	// Vendor is able to change the lines below to call optimized
	// DGEMM or other matrix multiplication routines. Do *NOT*
	// change any lines above this statement.
	// ------------------------------------------------------- //

	double sum = 0;

	// Repeat multiple times
	for(r = 0; r < repeats; r++) {
#if defined(USE_MKL) || defined(USE_CBLAS)
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
            N, N, N, alpha, matrixA, N, matrixB, N, beta, matrixC, N);
#elif USE_ESSL
        dgemm("N", "N",
            N, N, N, alpha, matrixA, N, matrixB, N, beta, matrixC, N);
#else
		#pragma omp parallel for private(sum)
		for(i = 0; i < N; i++) {
			for(j = 0; j < N; j++) {
				sum = 0;

				for(k = 0; k < N; k++) {
					sum += matrixA[i*N + k] * matrixB[k*N + j];
				}

				matrixC[i*N + j] = (alpha * sum) + (beta * matrixC[i*N + j]);
			}
		}
#endif
	}

	// ------------------------------------------------------- //
	// VENDOR NOTIFICATION: END MODIFIABLE REGION
	// ------------------------------------------------------- //

	// ------------------------------------------------------- //
	// DO NOT CHANGE CODE BELOW
	// ------------------------------------------------------- //

	const double end = get_seconds();

	printf("Calculating matrix check...\n");

	double final_sum = 0;
	double count     = 0;

	#pragma omp parallel for reduction(+:final_sum, count)
	for(i = 0; i < N; i++) {
		for(j = 0; j < N; j++) {
			final_sum += matrixC[i*N + j];
			count += 1.0;
		}
	}

	double N_dbl = (double) N;
	double matrix_memory = (3 * N_dbl * N_dbl) * ((double) sizeof(double));

	printf("\n");
	printf("===============================================================\n");

	printf("Final Sum is:         %f\n", (final_sum / (count * repeats)));
	printf("Memory for Matrices:  %f MB\n",
		(matrix_memory / (1024 * 1024)));

	const double time_taken = (end - start);

	printf("Multiply time:        %f seconds\n", time_taken);

	// O(N**3) elements each with one add and three multiplies
    	// (alpha, beta and A_i*B_i).
	const double flops_computed = (N_dbl * N_dbl * N_dbl * 2.0 * (double)(repeats)) +
        (N_dbl * N_dbl * 2 * (double)(repeats));

	printf("FLOPs computed:       %f\n", flops_computed);
	printf("GFLOP/s rate:         %f GF/s\n", (flops_computed / time_taken) / 1000000000.0);

	printf("===============================================================\n");
	printf("\n");

#if 0
	free(matrixA);
	free(matrixB);
	free(matrixC);
#endif

	return 0;
}
