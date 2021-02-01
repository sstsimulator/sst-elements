#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]) {

	int counter;
	const int n = 16;

	#pragma omp parallel private(counter)
	{
		for(counter = 0; counter < n; counter++) {
			#pragma omp barrier

			printf("Performing iteration %d\n", counter);
			fflush(stdout);

			#pragma omp barrier
		}
	}

}
