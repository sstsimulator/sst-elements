#include <stdio.h>
#include <omp.h>


int main(int argc, char* argv[]) {

	printf("OpenMP API Test\n");

	int thread_count = omp_get_max_threads();

	printf("Maximum number of OpenMP threads is: %d\n", thread_count);

	int thread_count_now = omp_get_num_threads();

	printf("There are %d threads active now (should equal 1 since not in parallel region)\n", thread_count_now);

	#pragma omp parallel
	{
		#pragma omp single
		{
			thread_count_now = omp_get_num_threads();
			printf("There are now %d threads active (should equal OMP_NUM_THREADS since in parallel region)\n", thread_count_now);
		}
	}

}
