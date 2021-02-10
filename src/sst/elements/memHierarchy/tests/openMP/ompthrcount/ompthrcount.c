#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char* argv[]) {

	int thread_count = 0;

	printf("Test will create many threads and increment a counter...\n");

	#pragma omp parallel 
	{
		#pragma omp atomic
		thread_count += 1;
	}

	printf("Hello OpenMP World just created: %d threads!\n",
		thread_count);

}
