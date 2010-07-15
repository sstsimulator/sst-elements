#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cprops/str.h>

#define PATH_SEPARATOR '/'

int main(int argc, char *argv[])
{
	int i;
	cp_string *s = cp_string_create("", 0);
	cp_string *s2, *s3;
	
	for (i = 0; i < argc; i++)
		cp_string_cstrcat(s, argv[i]);

	printf("cat: %s\n", cp_string_tocstr(s));

	cp_string_destroy(s);

	{
	cp_string *bp = cp_string_create(argv[0], strlen(argv[0]));
	printf("dump #1:\n");
	cp_string_dump(bp);
	if (argv[0][strlen(argv[0] - 1)] != PATH_SEPARATOR)
		cp_string_cstrcat(bp, "/");
	printf("concatenated: %s\n", cp_string_tocstr(bp));
	printf("dump #2:\n");
	cp_string_dump(bp);
	cp_string_destroy(bp);
	}

	s = cp_string_cstrdup("string One");
	s2 = cp_string_cstrdup("- string Two");
	s3 = cp_string_cstrdup("- string Three");
	cp_string_cat(s, s2);
	cp_string_cat(s, s3);

	printf("\nconcatenated again: %s\n", cp_string_tocstr(s));
	cp_string_dump(s);

	cp_string_destroy(s);
	cp_string_destroy(s2);
	cp_string_destroy(s3);

	return 0;
}

