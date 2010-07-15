#include <stdio.h>
#include <string.h>		/* for strdup */
#include <stdlib.h>		/* for free */
#include <errno.h>
#include <cprops/hashtable.h>

#ifdef CP_HAS_STRINGS_H
#include <strings.h>   	/* for strcasecmp */
#else
#include <cprops/util.h>
#endif

int main()
{
	cp_hashtable *t =
		cp_hashtable_create_by_option(COLLECTION_MODE_NOSYNC |
									  COLLECTION_MODE_COPY |
  									  COLLECTION_MODE_DEEP,
  									  10,
 									  cp_hash_istring,
  									  (cp_compare_fn) strcasecmp,
  									  (cp_copy_fn) strdup,
 									  (cp_destructor_fn) free,
  									  (cp_copy_fn) strdup,
									  (cp_destructor_fn) free);
	if (t == NULL)
	{
		perror("create");
		exit(1);
	}
		
	while (1)
	{
		char name[80];
		char number[80];
		
		printf("enter name [next]: ");
		name[0] = '\0';
		fgets(name, 80, stdin);
		name[strlen(name) - 1] = '\0'; /* chop newline */
		if (name[0] == '\0') break;
		
		printf("enter number [next]: ");
		number[0] = '\0';
		fgets(number, 80, stdin);
		number[strlen(number) - 1] = '\0'; /* chop newline */
		if (number[0] == '\0') break;

		cp_hashtable_put(t, name, number);

		printf("table size: %ld\n", t->table_size);
	} 
		
	while (1)
	{
		char name[80];
		char *number;
		
		printf("enter name [quit]: ");
		name[0] = '\0';
		fgets(name, 80, stdin);
		name[strlen(name) - 1] = '\0'; /* chop newline */
		if (name[0] == '\0') break;
		
		printf("contains: %d\n", cp_hashtable_contains(t, name));

		if ((number = cp_hashtable_get(t, name)) != NULL)
			printf("%s: %s\n", name, number);
		else
			printf("no entry for %s\n", name);
	}

	cp_hashtable_destroy(t);
	return 0;
}

