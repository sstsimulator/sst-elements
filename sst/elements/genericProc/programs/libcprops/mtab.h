#ifndef _CP_MTAB_H
#define _CP_MTAB_H

#include "common.h"
#include "collection.h"

__BEGIN_DECLS

#include "config.h"

/*
 * table entry descriptor. mtab entries map a character to a value, and in 
 * addition allow specifying an attribute for the mapping. this is used by 
 * cp_trie to collapse multiple single child trie edges into a single node.
 */
typedef struct _mtab_node
{
	unsigned char key;
	void *value;
	void *attr;
	struct _mtab_node *next;
} mtab_node;

mtab_node *mtab_node_new(unsigned char key, void *value, void *attr);

/* 
 * the 'owner' parameter is for use by the enclosing data structure. cp_trie
 * uses this to recursively delete a sub-tree.
 */
typedef void *(*mtab_dtr)(void *owner, mtab_node *node);

/*
 * mtab is a hash table implementation specialized for use as a holder for 
 * cp_trie edges. 
 */
typedef struct _mtab
{
	int size;
	int items;
	mtab_node **table;
} mtab;

mtab *mtab_new(int size);

void mtab_delete(mtab *t);
void mtab_delete_custom(mtab *t, 
		                void *owner, 
						mtab_dtr dtr);
mtab_node *mtab_put(mtab *t, unsigned char key, void *value, void *attr);
mtab_node *mtab_get(mtab *t, unsigned char key);
void *mtab_remove(mtab *t, unsigned char key);

int mtab_count(mtab *t);

int mtab_callback(mtab *t, cp_callback_fn fn, void *prm);

__END_DECLS

#endif
