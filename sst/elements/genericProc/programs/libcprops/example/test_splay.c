#include <stdio.h>
#include <string.h>       /* for strdup */
#include <stdlib.h>       /* for free */
#include <errno.h>
#include <cprops/splay.h>

#ifdef CP_HAS_STRINGS_H
#include <strings.h>      /* for strcasecmp */
#else
#include <cprops/util.h>
#endif /* CP_HAS_STRINGS_H */

int main()
{
	int opcount = 0;
	int insert_count = 0;
	int delete_count = 0;
	int ncount;
	cp_splaytree *t =
		cp_splaytree_create_by_option(COLLECTION_MODE_NOSYNC | 
									COLLECTION_MODE_COPY |
									COLLECTION_MODE_DEEP, 
									(cp_compare_fn) strcmp, 
									(cp_copy_fn) strdup, free, 
									(cp_copy_fn) strdup, free);
	
	if (t == NULL)
	{
		perror("create");
		exit(1);
	}

	while (++opcount)
   	{
		char *x;
		char name[80];
//		char number[80];

		printf("[== %d ==] enter name [next]: ", opcount);
		name[0] = '\0';
		fgets(name, 80, stdin);
		name[strlen(name) - 1] = '\0'; /* chop newline */
		if (name[0] == '\0') break;
#if 0
		printf("enter number [next]: ");
		number[0] = '\0';
		fgets(number, 80, stdin);
		number[strlen(number) - 1] = '\0'; /* chop newline */
		if (number[0] == '\0') break;

		cp_splaytree_insert(t, strdup(name), strdup(number));
#endif
		x = cp_splaytree_get(t, name);
		if (x == NULL)
		{
			printf("INSERT %s\n", name);
			cp_splaytree_insert(t, name, name);
			insert_count++;
		}
		else
		{
			printf("DELETE %s\n", name);
			cp_splaytree_delete(t, name);
			delete_count++;
		}

		cp_splaytree_dump(t);
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

		number = cp_splaytree_get(t, name);

		cp_splaytree_dump(t);

		if (number != NULL)
			printf("%s: %s\n", name, number);
		else
			printf("no entry for %s\n", name);
	}

	cp_splaytree_destroy(t);
	return 0;
}

