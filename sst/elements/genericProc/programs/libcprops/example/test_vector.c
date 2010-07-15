#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cprops/vector.h>

int main()
{
	int i, n;
	char *a = "AaAa";
	char *b = "BbBb";
	char *c = "CcCc";
	char *p;
	
	cp_vector *v = cp_vector_create(1);

	for (i = 0; i < 30; i++)
	{
		char buf[0x20];
		sprintf(buf, "%s %d", a, i);
		cp_vector_add_element(v, strdup(buf));
		sprintf(buf, "%s %d", b, i);
		cp_vector_add_element(v, strdup(buf));
		sprintf(buf, "%s %d", c, i);
		cp_vector_add_element(v, strdup(buf));
	}

	n = cp_vector_size(v);

	for (i = 0; i < n; i++)
	{
		p = cp_vector_element_at(v, i);
		printf("%d: %s\n", i, p);
	}

	cp_vector_destroy_custom(v, free);
	
	return 0;
}

