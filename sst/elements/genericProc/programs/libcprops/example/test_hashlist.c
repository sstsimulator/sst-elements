#include <cprops/hashlist.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	char key[32];
	char value[32];
	char *res;
	cp_hashlist *t;
	cp_hashlist_iterator *i;
	int j;

	t = cp_hashlist_create_by_option(COLLECTION_MODE_NOSYNC | 
							    	 COLLECTION_MODE_DEEP |
									 COLLECTION_MODE_COPY,
									 10,
									 cp_hash_string,
									 (cp_compare_fn) strcmp,
									 (cp_copy_fn) strdup,
                                     (cp_destructor_fn) free,
									 (cp_copy_fn) strdup,
                                     (cp_destructor_fn) free);
	if (t == NULL)
	{
		perror(argv[0]);
		exit(1);
	}

	for (j = 0; j < 10; j++)
	{
		sprintf(key, "ENTRY (%d)", j);
		sprintf(value, "VALUE (%d)", j);
		if (cp_hashlist_insert(t, key, value) == NULL)
		{
			perror(argv[0]);
			exit(1);
		}
	}
	
	res = cp_hashlist_get(t, "ENTRY (5)");
	printf("looking up entry #5: %s\n", res ? res : "not found");

	printf("iterating over list values:\n");
	i = cp_hashlist_create_iterator(t, COLLECTION_LOCK_NONE);
	while ((res = cp_hashlist_iterator_next_value(i)))
		printf("\to\t%s\n", res);

	cp_hashlist_iterator_destroy(i);
	cp_hashlist_destroy(t);

	return 0;
}

