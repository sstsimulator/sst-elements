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
