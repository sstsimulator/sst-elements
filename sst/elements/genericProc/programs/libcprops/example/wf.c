/* wf.c - list distinct words by frequency
 *
 * wf expects a filename to read words from as its single parameter, or reads 
 * from the standard input if no filename is given. Words occurences are
 * counted in a mapping collection (wordlist). This results in a mapping of
 * distinct words to frequencies, sorted alphabetically. By means of a 
 * callback iteration, the mappings are then transferred to another mapping 
 * collection (freqlist) where they are stored by frequency. This results in a
 * frequency sorted list which is then printed out. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>       /* for strdup, strpbrk */
#include <cprops/avl.h>   /* for binary tree */
#include <cprops/str.h>   /* for string functions */
#include <cprops/util.h>  /* for intdup, filter_string */

/* sort function for frequency list */
int freqcmp(int *a, int *b)
{
	return *b - *a;
}

/* callback for building frequency list from word list */
int add_freq(void *entry, void *tree)
{
	cp_avltree *freq = tree;
	cp_avlnode *node = entry;
	cp_avltree_insert(freq, node->value, node->key);

	return 0;
}

/* output callback */
int print_word_freq(void *entry, void *ordp)
{
	cp_avlnode *n = entry;
	int *count = n->key;
	cp_vector *res = n->value;
	int *ord = ordp;
	int i;

	for (i = 0; i < cp_vector_size(res); i++)
		printf("(%d) [%d] %s\n", *ord, *count, 
				(char *) cp_vector_element_at(res, i));
	(*ord)++;

	return 0;
}

#define PUNCT ".,!?;:\'\"~()\\/-[]<>"

int main(int argc, char *argv[])
{
	FILE *in = stdin;
	char buf[0x4000]; /* limited to 16K character word length */
	cp_avltree *wordlist;
	cp_avltree *freqlist;
	int count = 0;
	int one = 1;
	int *wc;

	if (argc > 1) 
	{
		in = fopen(argv[1], "r");
		if (in == NULL)
		{
			fprintf(stderr, "can\'t open %s\n", argv[1]);
			exit(1);
		}
	}
	
	/* wordlist keys are distinct */
	wordlist = 
		cp_avltree_create_by_option(COLLECTION_MODE_NOSYNC | 
									COLLECTION_MODE_COPY | 
									COLLECTION_MODE_DEEP, 
									(cp_compare_fn) strcmp, 
									(cp_copy_fn) strdup, free, 
									(cp_copy_fn) intdup, free);
	
	while ((fscanf(in, "%s", buf)) != EOF)
	{
		filter_string(buf, PUNCT);
		to_lowercase(buf);
		wc = cp_avltree_get(wordlist, buf);
		if (wc) 
			(*wc)++;
		else
			cp_avltree_insert(wordlist, buf, &one);
		count++;
	}

	fclose(in);

	/* frequency list entry keys are not distinct */
	freqlist = 	
		cp_avltree_create_by_option(COLLECTION_MODE_NOSYNC | 
									COLLECTION_MODE_MULTIPLE_VALUES,
									(cp_compare_fn) freqcmp, 
									NULL, NULL, NULL, NULL);
	
	cp_avltree_callback(wordlist, add_freq, freqlist);
	cp_avltree_callback(freqlist, print_word_freq, &one);
	printf("%d words, %d distinct entries\n", count, cp_avltree_count(wordlist));

	cp_avltree_destroy(freqlist);
	cp_avltree_destroy(wordlist);

	return 0;
}

