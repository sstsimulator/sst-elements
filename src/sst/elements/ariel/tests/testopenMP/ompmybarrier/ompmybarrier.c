#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char* argv[]) {

	int counter;
	const int n = 16;
        int id;

	#pragma omp parallel private(counter, id)
	{
                id = omp_get_thread_num();
		for(counter = 0; counter < n; counter++) {
			#pragma omp barrier

			printf("%d Performing iteration %d\n", id, counter);
			fflush(stdout);

			#pragma omp barrier
		}
	}

}
