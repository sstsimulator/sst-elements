#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cprops/heap.h>
#include <cprops/log.h>
#include <cprops/util.h>
#include <cprops/hashtable.h>

int main(int argc, char *argv[])
{
	int *i;
	int j, val;
	int max = 0;
	int log_level = 0;
	cp_heap *heap;

	if (argc > 1) max = atoi(argv[1]);
	if (max == 0) max = 10;

	if (argc > 2) log_level = atoi(argv[2]);

	cp_log_init("heap-test.log", log_level);
	
	heap = cp_heap_create(cp_hash_compare_int);

	srand(time(NULL));

	for (j = 0; j < max; j++)
	{
		val = rand();
		cp_heap_push(heap, intdup(&val));
	}
		
	while ((i = cp_heap_pop(heap)))
	{
		printf("%d\n", *i);
		max--;
		if (max == 50) cp_heap_contract(heap);
		cp_free(i);
	}

	cp_heap_contract(heap);

	cp_heap_destroy(heap);

	cp_log_close();

	return 0;
}

