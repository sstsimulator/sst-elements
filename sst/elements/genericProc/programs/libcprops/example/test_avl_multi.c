#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cprops/vector.h>
#include <cprops/avl.h>

void print_result(cp_vector *v, char *title)
{
	int i;

	printf("%s:\n", title);

	for (i = 0; i < cp_vector_size(v); i++)
		printf("%s\n", (char *) cp_vector_element_at(v, i));
}

int main(int argc, char *argv[])
{
	int i;
	cp_vector *res;
	cp_avltree *t;

	t = cp_avltree_create_by_option(COLLECTION_MODE_COPY | 
									COLLECTION_MODE_DEEP |
						  			COLLECTION_MODE_NOSYNC | 
									COLLECTION_MODE_MULTIPLE_VALUES, 
									(cp_compare_fn) strcmp, 
									(cp_copy_fn) strdup, free, 
									(cp_copy_fn) strdup, free);

	cp_avltree_insert(t, "animal", "winnie");
	cp_avltree_insert(t, "animal", "moby");
	cp_avltree_insert(t, "vegetable", "carrot");
	cp_avltree_insert(t, "vegetable", "tomato");

	res = cp_avltree_get(t, "animal");
	print_result(res, "animals");
	
	res = cp_avltree_get(t, "vegetable");
	print_result(res, "vegetables");

	printf("\nbefore:\n");
	cp_avltree_dump(t);

	printf("\ndelete vegetables:\n");
	res = cp_avltree_delete(t, "vegetable");
	cp_avltree_dump(t);

	printf("\ndelete animals:\n");
	res = cp_avltree_delete(t, "animal");
	cp_avltree_dump(t);
	
	cp_avltree_destroy(t);

	return 0;
}

