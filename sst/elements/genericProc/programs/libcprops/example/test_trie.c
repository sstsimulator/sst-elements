#include <stdio.h>
#include <stdlib.h>
#include <cprops/trie.h>

int main()
{
	void *prm;
	char *v;
	cp_trie *t = cp_trie_create(0);

	if (t == NULL)
	{
		printf("can\'t create trie\n");
		exit(1);
	}
	
	cp_trie_add(t, "ferguson", "xxx");
	cp_trie_add(t, "fermat", "yyy");

	cp_trie_prefix_match(t, "ferry", &prm);
	v = prm;
	printf("prefix match for ferry: %s\n", v ? v : "none");

	v = cp_trie_exact_match(t, "ferry");
	printf("exact match for ferry: %s\n", v ? v : "none");

	cp_trie_prefix_match(t, "fergusonst", &prm);
	v = prm;
	printf("prefix match for fergusonst: %s\n", v ? v : "none");

	v = cp_trie_exact_match(t, "fergusonst");
	printf("exact match for fergusonst: %s\n", v ? v : "none");

	cp_trie_destroy(t);

	return 0;
}
