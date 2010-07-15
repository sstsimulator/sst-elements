#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cprops/collection.h>
#include <cprops/hashlist.h>

void print_and_free(void *ptr)
{
	char *txt = (char *) ptr;

	printf(" -> %s", txt);
	free(txt);
}

int print_entry(void *key, void *value, void *dummy)
{
	printf(" - (%s => %s) - ", (char *) key, (char *) value);
	return 0;
}


int main(int argc, char *argv[])
{
	int i;
	char buf[0x20];
	cp_hashlist_iterator itr;
	char *item;
	cp_hashlist *l;
	
	printf("%s: starting\n", argv[0]);
	
	l = cp_hashlist_create_by_option(COLLECTION_MODE_NOSYNC | 
								     COLLECTION_MODE_COPY | 
									 COLLECTION_MODE_DEEP,
								   	 10, 
									 cp_hash_string, 
									 cp_hash_compare_string,
									 (cp_copy_fn) strdup,
									 (cp_destructor_fn) free,
									 (cp_copy_fn) strdup,
									 (cp_destructor_fn) free);
	
	printf("%s: created (%lX)\n", argv[0], (long) l);

	for (i = 0; i < 10; i++)
	{
		sprintf(buf, "item #%d", i);
		printf("%s: adding [%s => %s]\n", argv[0], buf, buf);
		cp_hashlist_append(l, buf, buf);
	}

	printf("list: %lX list head: %lX list tail: %lX\n", (long) l, l ? (long) l->head : -1L, l ? (long) l->tail : -1L);

	printf("--- initial state: \n");
	cp_hashlist_callback(l, print_entry, NULL);
	printf("\n\n");
	
	printf("list: %lX list head: %lX list tail: %lX\n", (long) l, l ? (long) l->head : -1L, l ? (long) l->tail : -1L);
	printf("--- attempting middle insertion\n");
	cp_hashlist_iterator_init(&itr, l, COLLECTION_LOCK_NONE);
	while ((item = cp_hashlist_iterator_next_key(&itr)))
	{
		printf("checking key [%s]", item);
		if (strcmp(item, "item #8") == 0)
		{
			printf("inserting... ");
			cp_hashlist_iterator_insert(&itr, "in between", "8 & 9");
		}
		printf("\n");
	}

	printf("--- after iterator insertion in the middle: \n");
	cp_hashlist_callback(l, print_entry, NULL);
	printf("\n\n");
			
	printf("inserting post iteratum:\n");
	cp_hashlist_iterator_insert(&itr, "at the end", "fin");
	printf("--- after iterator insertion at the end: \n");
	cp_hashlist_callback(l, print_entry, NULL);
	printf("\n\n");
			
	printf("iterating backwards:\n");
	while ((item = cp_hashlist_iterator_prev_key(&itr)))
		printf(" << %s ", (char *) item);

	cp_hashlist_iterator_insert(&itr, "up front", "primo");
	printf("--- after iterator insertion at the beginning: \n");
	cp_hashlist_callback(l, print_entry, NULL);
	printf("\n\n");

	if (cp_hashlist_iterator_to_key(&itr, "item #3") == 0)
	{
		cp_hashlist_iterator_insert(&itr, "before III", "2.5");
		cp_hashlist_iterator_append(&itr, "after III", "3.5");
		printf("--- after insertions at III: \n");
		cp_hashlist_callback(l, print_entry, NULL);
		printf("\n\n");
	}
	else
		printf("missing item #3\n");

	cp_hashlist_iterator_next(&itr);
	cp_hashlist_iterator_next(&itr);
	cp_hashlist_iterator_next(&itr);
	cp_hashlist_iterator_remove(&itr);
	cp_hashlist_iterator_remove(&itr);
	printf("--- next x 3, remove x 2: \n");
	cp_hashlist_callback(l, print_entry, NULL);
	printf("\n\n");
	
	cp_hashlist_destroy_custom(l, print_and_free, print_and_free);
	printf("\n\n");

	return 0;
}

