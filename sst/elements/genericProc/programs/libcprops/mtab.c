#include <stdlib.h>
#include "mtab.h"

/*
 * mtab.c - a hash table implementation designed for use as node link storage
 * in cp_trie objects. performance benefits compared to cp_hashtable result 
 * from cutting the overhead of dereferencing generic hash and compare 
 * functions and saving on checks for collection mode.
 */

/* an array of prime numbers for hash table sizes */
static int sizes[] = {   1,   3,   5,   7,  11,  19,  29,  37,  47,  59,
						71,  89, 107, 127, 151, 181, 211, 239, 257, 281 };

static int sizes_len = 20;

#define MIN_FILL_FACTOR 30
#define MAX_FILL_FACTOR 100
#define DOWNSIZE_RATIO   2

static unsigned long mt_abs(long x) 
{
	if (x < 0) return -x;
	return x;
}

/*
 * performs a binary search on the sizes array to choose the first entry larger
 * than the requested size. the sizes array is initialized with prime numbers.
 */
static int choose_size(int size)
{
    int new_size;

    if (sizes[sizes_len - 1] < size)
    {
        for (new_size = sizes[sizes_len - 1];
             new_size < size;
             new_size = new_size * 2 + 1);
    }
    else
    {
        int min = -1;
        int max = sizes_len - 1;
        int pos;

        while (max > min + 1)
        {
            pos = (max + min + 1) / 2;
            if (sizes[pos] < size)
                min = pos;
            else
                max = pos;
        }

        new_size = sizes[max];
    }

    return new_size;
}

static void resize_table(mtab *t, int size)
{
	mtab_node **table = (mtab_node **) calloc(size, sizeof(mtab_node *));

	if (table)
	{
		int i;
		mtab_node *ni;
		mtab_node **nf;
		for (i = 0; i < t->size; i++)
		{
			ni = t->table[i];
			while (ni)
			{
				nf = &table[ni->key % size];
				while (*nf) nf = &(*nf)->next;
				*nf = ni;
				ni = ni->next;
				(*nf)->next = NULL;
			}
		}

		free(t->table);
		t->table = table;
		t->size = size;
	}
}

mtab_node *mtab_node_new(unsigned char key, void *value, void *attr)
{
	mtab_node *node = calloc(1, sizeof(mtab_node));
	if (node)
	{
		node->key = key;
		node->value = value;
		node->attr = attr;
	}

	return node;
}


mtab *mtab_new(int size)
{
	mtab *t = calloc(1, sizeof(mtab));
	if (t)
	{
		t->size = choose_size(size);
		t->table = calloc(t->size, sizeof(mtab_node *));
		if (t->table == NULL)
		{
			free(t);
			t = NULL;
		}
	}

	return t;
}

/*
 * the 'owner' pointer is used by cp_trie deletion code to pass a pointer to the 
 * trie being deleted for access to mode settings etc.
 */
void mtab_delete_custom(mtab *t, 
		                void *owner, 
						mtab_dtr dtr)
{
	while (t->size--)
	{
		mtab_node *curr = t->table[t->size];
		mtab_node *tmp;
		while (curr)
		{
			tmp = curr;
			curr = curr->next;
			(*dtr)(owner, tmp);
			free(tmp);
		}
	}

	free(t->table);
	free(t);
}

void mtab_delete(mtab *t)
{
	while (t->size--)
	{
		mtab_node *curr = t->table[t->size];
		mtab_node *tmp;
		while (curr)
		{
			tmp = curr;
			curr = curr->next;
			free(tmp);
		}
	}

	free(t->table);
	free(t);
}

/*
 * mtab doesn't support collection modes. inserting a new value for an 
 * key already present silently replaces the existing value.
 */
mtab_node *mtab_put(mtab *t, unsigned char key, void *value, void *attr)
{
	mtab_node **loc;
		
	if ((t->items + 1) * 100 > t->size * MAX_FILL_FACTOR) 
		resize_table(t, choose_size(t->size + 1));
	
	loc = &t->table[key % t->size];
	while (*loc && (*loc)->key != key) loc = &(*loc)->next;

	if (*loc == NULL)
	{
		t->items++;
		*loc = mtab_node_new(key, value, attr);
	}
	else
	{
		(*loc)->value = value; /* replace */
		if ((*loc)->attr) free((*loc)->attr);
		(*loc)->attr = attr;
	}

	return *loc;
}

mtab_node *mtab_get(mtab *t, unsigned char key)
{
	mtab_node *node = t->table[key % t->size];

	while (node && key != node->key)
		node = node->next;

	return node;
}

/*
 * mtab_remove performs a table resize if table density drops below the 'fill
 * ratio'. this is done to keep cp_trie memory usage low.
 */
void *mtab_remove(mtab *t, unsigned char key)
{
	mtab_node **node;
	void *res = NULL;

	node = &t->table[key % t->size];

	while ((*node) && key != (*node)->key)
		node = &(*node)->next;

	if (*node)
	{
		mtab_node *rm = *node;
		*node = rm->next; /* unlink */
		res = rm->value;
		if (rm->attr) free(rm->attr);
		free(rm);

		t->items--;
	}

	if (t->items > 0 && t->items * 100 < t->size * MIN_FILL_FACTOR)
		resize_table(t, choose_size(t->size / DOWNSIZE_RATIO));

	return res;
}

int mtab_count(mtab *t)
{
	return t->items;
}

int mtab_callback(mtab *t, cp_callback_fn fn, void *prm)
{
	int i;
	mtab_node *n;
	int count = 0;
	int res = 0;

	for (i = 0; i < t->size && count < t->items; i++)
	{
		n = t->table[i];
		while (n)
		{
			count++;
			if ((res = (*fn)(n, prm)) != 0) return res;
			n = n->next;
		}
	}

	return 0;
}

