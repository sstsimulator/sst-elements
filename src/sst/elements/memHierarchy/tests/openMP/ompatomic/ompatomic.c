#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]) {

	int thread_count = 0;

	#pragma omp parallel
	{
		#pragma omp atomic
		thread_count += 1;
	}

	printf("Counted: %d threads\n", thread_count);

}
