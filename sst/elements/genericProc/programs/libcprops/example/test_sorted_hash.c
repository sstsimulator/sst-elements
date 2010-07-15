#include <stdio.h>
#include <string.h>     /* for strdup */
#include <stdlib.h>     /* for free */
#include <errno.h>
#include <cprops/sorted_hash.h>

#ifdef CP_HAS_STRINGS_H
#include <strings.h>    /* for strcasecmp */
#else
#include <cprops/util.h>
#endif /* CP_HAS_STRINGS_H */

int compare_timeout(cp_mapping *n, cp_mapping *m)
{
	int *ni = (int *) cp_mapping_value(n);
	int *mi = (int *) cp_mapping_value(m);

	return *ni - *mi;
}

int print_timeout(void *mapping, void *prm)
{
	cp_mapping *m = (cp_mapping *) mapping;
	printf("[%s] %d\n", m->key, *(int *) m->value);
	return 0;
}

int main()
{
	int opcount = 0;
	int insert_count = 0;
	int delete_count = 0;
	int ncount;
	cp_sorted_hash *t =
	    cp_sorted_hash_create_by_option(COLLECTION_MODE_COPY | 
				                   		COLLECTION_MODE_DEEP | 
								   		COLLECTION_MODE_NOSYNC,
										10,
										cp_hash_string, 
				                   		(cp_compare_fn) strcmp,
										compare_timeout, 
								   		(cp_copy_fn) strdup, free, 
								   		(cp_copy_fn) intdup, free);
	
	if (t == NULL)
	{
		perror("create");
		exit(1);
	}

	while (++opcount)
   	{
		char *x;
		char name[80];
		int delay;
		char number[80];

		printf("[== %d ==] enter name [next]: ", opcount);
		name[0] = '\0';
		fgets(name, 80, stdin);
		name[strlen(name) - 1] = '\0'; /* chop newline */
		if (name[0] == '\0') break;

		printf("enter delay [next]: ");
		number[0] = '\0';
		fgets(number, 80, stdin);
		number[strlen(number) - 1] = '\0'; /* chop newline */
		if (number[0] == '\0') break;
		delay = atoi(number);

		x = cp_sorted_hash_get(t, name);
		if (x == NULL)
		{
			printf("INSERT %s\n", name);
			cp_sorted_hash_insert(t, name, &delay);
			insert_count++;
		}
		else
		{
			printf("DELETE %s\n", name);
			cp_sorted_hash_delete(t, name);
			delete_count++;
		}

		printf("\nsize: %d +%d -%d\n", cp_sorted_hash_count(t), insert_count, delete_count);

		cp_sorted_hash_callback(t, print_timeout, NULL);
	}

	while (1)
	{
		char name[80];
		int *number;

		printf("enter name [quit]: ");
		name[0] = '\0';
		fgets(name, 80, stdin);
		name[strlen(name) - 1] = '\0'; /* chop newline */
		if (name[0] == '\0') break;

		printf("contains: %d\n", cp_sorted_hash_contains(t, name));

		if ((number = cp_sorted_hash_get(t, name)) != NULL)
			printf("%s: %d\n", name, *number);
		else
			printf("no entry for %s\n", name);
	}

	cp_sorted_hash_callback(t, print_timeout, NULL);
	cp_sorted_hash_destroy(t);
	return 0;
}

