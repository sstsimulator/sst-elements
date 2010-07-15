#include <stdio.h>
#include <string.h>
#include <stdlib.h>             /* for free */
#include <errno.h>
#include <cprops/trie.h>

#include <cprops/config.h>
#ifdef CP_HAS_STRINGS_H
#include <strings.h>            /* for strcmp */
#else
#include <cprops/util.h>
#endif /* CP_HAS_STRINGS_H */

// #define cp_trie_count(t) ((t)->path_count)

int main()
{
	cp_trie *t =
		cp_trie_create_trie(COLLECTION_MODE_NOSYNC | 
							COLLECTION_MODE_COPY | COLLECTION_MODE_DEEP,
							(cp_copy_fn) strdup, free);
	
	if (t == NULL)
	{
		perror("create");
		exit(1);
	}

	while (1)
   	{
		char *x;
		char name[80];
//		char number[80];

		printf("enter name [next]: ");
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

		cp_trie_add(t, name, strdup(number));
#endif
		x = cp_trie_exact_match(t, name);
printf("match for %s: %s\n", name, x ? x : "none");
		if (x == NULL)
			cp_trie_add(t, name, name);
		else
			cp_trie_remove(t, name, NULL);

		printf("\nsize: %d\n", cp_trie_count(t));
		cp_trie_dump(t);
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

		printf("contains: %d\n", cp_trie_exact_match(t, name) != NULL);

		if ((number = cp_trie_exact_match(t, name)) != NULL)
			printf("%s: %s\n", name, number);
		else
			printf("no entry for %s\n", name);
	}

	cp_trie_destroy(t);
	return 0;
}

