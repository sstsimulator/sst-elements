#include <stdio.h>
#include <stdlib.h>

#include <cprops/priority_list.h>
#include <cprops/thread.h>

#define MAX_ENTRIES 100

static cp_priority_list *l;
static int done;

void *read_fn(void *list)
{
	cp_priority_list *l = (cp_priority_list *) list;
	char *item;

	while (!done || cp_priority_list_item_count(l))
	{
		while (cp_priority_list_item_count(l))
		{
			item = cp_priority_list_get_next(l);
			printf("[%ld] %s\n", cp_priority_list_item_count(l), item);
			free(item);
		}
	}

	return NULL;
}


int main(int argc, char *argv[])
{
	cp_thread reader;

	int i;
	int weights[] = {5, 3};
	char *label[] = {"immediate %d ser %d", 
				     "high      %d ser %d", 
					 "low       %d ser %d"};
	int counter[3] = {0, 0, 0};
	char *item;

	int max_entries;

	srand(time(NULL)); /* that's right */
	max_entries = argc > 1 ? atoi(argv[1]) : MAX_ENTRIES;
	
	l = cp_priority_list_create(1, 2, weights);

	done = 0;
	cp_thread_create(reader, NULL, read_fn, l);
	
	for (i = 0; i < max_entries; i++)
	{
		int rnd = rand() % 3;
		item = (char *) malloc(64 * sizeof(char));
		sprintf(item, label[rnd], counter[rnd]++, i);
		printf("[%ld] %s\n", cp_priority_list_item_count(l), item);
			
		cp_priority_list_insert(l, item, rnd);
	}

	done = 1;

	printf("pushed %d entries (%d, %d, %d)\n", i, counter[0], counter[1], counter[2]);
	
	cp_thread_join(reader, NULL);

	cp_priority_list_destroy(l);

	return 0;
}

