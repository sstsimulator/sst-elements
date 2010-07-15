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

int main(int argc, char *argv[])
{
	int i;
	char buf[0x20];

	cp_list *l = 
		cp_list_create_list(COLLECTION_MODE_COPY | 
				            COLLECTION_MODE_MULTIPLE_VALUES, 
							(cp_compare_fn) strcmp, (cp_copy_fn) strdup, NULL);
	
	for (i = 0; i < 10; i++)
	{
		sprintf(buf, "item #%d", i);
		cp_list_append(l, buf);
	}

	cp_list_insert_after(l, "after 8", "item #8");
	cp_list_insert_before(l, "before 9", "item #9");
	cp_list_insert_after(l, "after none", "none");
	cp_list_insert_before(l, "before none", "none");

	cp_list_destroy_custom(l, print_and_free);

	printf("\n");

	return 0;
}

