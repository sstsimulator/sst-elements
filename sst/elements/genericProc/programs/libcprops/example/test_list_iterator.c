#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cprops/linked_list.h>

void print_and_free(void *ptr)
{
	char *txt = (char *) ptr;

	printf(" -> %s", txt);
	free(txt);
}

int print_item(void *item, void *dummy)
{
	printf(" - %s - ", (char *) item);
	return 0;
}

#if 0
int cp_list_callback(cp_list *l, int (*item_action)(void *))
{
	int rc = 0;
	cp_list_entry *curr;

	if (!(l->mode & COLLECTION_MODE_NOSYNC))
		rc = cp_list_rdlock(l);

	if (rc) return rc;

	for (curr = l->head; curr; curr = curr->next)
		if ((rc = (*item_action)(curr->item)))
			break;

	if (!(l->mode & COLLECTION_MODE_NOSYNC))
		cp_list_unlock(l);

	return rc;
}
#endif

int main(int argc, char *argv[])
{
	int i;
	char buf[0x20];
	cp_list_iterator itr;
	char *item;

	cp_list *l = 
		cp_list_create_list(COLLECTION_MODE_NOSYNC | 
							COLLECTION_MODE_COPY | 
							COLLECTION_MODE_DEEP |
							COLLECTION_MODE_MULTIPLE_VALUES, 
							(cp_compare_fn) strcmp, (cp_copy_fn) strdup, free);
	
	for (i = 0; i < 10; i++)
	{
		sprintf(buf, "item #%d", i);
		cp_list_append(l, buf);
	}

	printf("--- initial state: \n");
	cp_list_callback(l, print_item, NULL);
	printf("\n\n");
	
	cp_list_insert_after(l, "after 8", "item #8");
	cp_list_insert_before(l, "before 9", "item #9");
	cp_list_insert_after(l, "after none", "none");
	cp_list_insert_before(l, "before none", "none");

	printf("--- after insertions: \n");
	cp_list_callback(l, print_item, NULL);
	printf("\n\n");

	cp_list_iterator_init(&itr, l, COLLECTION_LOCK_NONE);
	while ((item = cp_list_iterator_next(&itr)))
	{
		if (strcmp(item, "after 8") == 0)
		{
			printf("inserting... ");
			cp_list_iterator_insert(&itr, "in between");
		}
	}

	printf("--- after iterator insertion in the middle: \n");
	cp_list_callback(l, print_item, NULL);
	printf("\n\n");
			
	cp_list_iterator_insert(&itr, "at the end");
	printf("--- after iterator insertion at the end: \n");
	cp_list_callback(l, print_item, NULL);
	printf("\n\n");
			
	printf("iterating backwards:\n");
	while ((item = cp_list_iterator_prev(&itr)))
		printf(" << %s ", (char *) item);

	cp_list_iterator_insert(&itr, "up front");
	printf("--- after iterator insertion at the beginning: \n");
	cp_list_callback(l, print_item, NULL);
	printf("\n\n");

	cp_list_iterator_next(&itr);
	cp_list_iterator_next(&itr);
	cp_list_iterator_next(&itr);
	cp_list_iterator_remove(&itr);
	cp_list_iterator_remove(&itr);
	cp_list_iterator_remove(&itr);
	printf("--- next x 3, remove x 2: \n");
	cp_list_callback(l, print_item, NULL);
	printf("\n\n");
	
	cp_list_destroy_custom(l, print_and_free);
	printf("\n\n");

	return 0;
}

