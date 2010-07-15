#ifndef _SORTED_HASH_H
#define _SORTED_HASH_H

/** @{ */
/**
 * @file
 *
 * sorted hash table - not unlike cp_hashlist, only nodes are arranged in a
 * red black tree to allow ordered traversal. Hence a user defined ordering is
 * conserved rather than insertion order. 
 *
 * cp_sorted_hash allows hash table like lookups using the given keys. Sorting
 * is done on a cp_mapping structure, so that the comparison function can 
 * examine key, value or both.
 */

#include "common.h"

__BEGIN_DECLS

#include "collection.h"
#include "vector.h"
#include "mempool.h"

struct _cp_sorted_hash;

#define SH_RED    0
#define SH_BLACK  1

typedef CPROPS_DLL struct _cp_sh_entry
{
	void *key;
	void *value;

	unsigned long code; /* hash code */

	/* balance maintainance - color is either 'red' or 'black' */
	int color;

	struct _cp_sh_entry *bucket;	/* list of nodes w/ same hash code */
	struct _cp_sh_entry *multiple;  /* list of nodes w/ same compare value */
	struct _cp_sh_entry *multiple_prev; /* reverse multiple list for removal */

	struct _cp_sh_entry *left;
	struct _cp_sh_entry *right;
	struct _cp_sh_entry *up;
} cp_sh_entry;

/* (internal) allocate a new node */
CPROPS_DLL
cp_sh_entry *cp_sh_entry_create(void *key, void *value, cp_mempool *pool);
/* (internal) deallocate a node */
CPROPS_DLL
void cp_sorted_hash_destroy_entry(struct _cp_sorted_hash *owner, 
								  cp_sh_entry *entry);
/* (internal) deallocate a node and its subnodes */
CPROPS_DLL
void cp_sorted_hash_destroy_entry_deep(struct _cp_sorted_hash *owner, 
									   cp_sh_entry *entry);

/* tree wrapper object */
typedef CPROPS_DLL struct _cp_sorted_hash
{
	cp_sh_entry *root;      	     /* root node */
	cp_sh_entry **table; 			 /* entry table */
	int size;						 /* entry table size */
	
	int items;                  	 /* item count */

	int mode;						 /* mode flags */
	cp_hashfunction hash;			 /* hash function */
	cp_compare_fn cmp_key; 			 /* key comparison function */
	cp_mapping_cmp_fn cmp_mapping;   /* item comparison function */
	cp_copy_fn key_copy;         	 /* key copy function */
	cp_destructor_fn key_dtr;    	 /* key destructor */
	cp_copy_fn value_copy;       	 /* value copy function */
	cp_destructor_fn value_dtr;  	 /* value destructor */

	cp_lock *lock;
	cp_thread txowner;           	 /* set if a transaction is in progress */
	int txtype;                  	 /* lock type */

	cp_mempool *mempool; 		 	 /* optional memory pool */
} cp_sorted_hash;

/* 
 * default create function - equivalent to create_by_option with mode 
 * COLLECTION_MODE_NOSYNC
 */
CPROPS_DLL
cp_sorted_hash *cp_sorted_hash_create(cp_hashfunction hash, 
									  cp_compare_fn cmp_key, 
									  cp_mapping_cmp_fn cmp_mapping);
/*
 * complete parameter create function. Note that setting COLLECTION_MODE_COPY
 * without specifying a copy function for either keys or values will result in
 * keys or values respectively being inserted by value, with no copying 
 * performed. Similarly, setting COLLECTION_MODE_DEEP without specifying a 
 * destructor function for keys or values will result in no destructor call
 * for keys or values respectively. This allows using the copy/deep mechanisms
 * for keys only, values only or both.
 */
CPROPS_DLL
cp_sorted_hash *
	cp_sorted_hash_create_by_option(int mode, 
									unsigned long size_hint,
									cp_hashfunction hash, 
									cp_compare_fn cmp_key, 
									cp_mapping_cmp_fn cmp_mapping, 
									cp_copy_fn key_copy, 
									cp_destructor_fn key_dtr,
							   		cp_copy_fn val_copy, 
									cp_destructor_fn val_dtr);

/* 
 * recursively destroy the tree structure 
 */
CPROPS_DLL
void cp_sorted_hash_destroy(cp_sorted_hash *tree);
/*
 * recursively destroy the tree structure with the given destructor functions
 */
CPROPS_DLL
void cp_sorted_hash_destroy_custom(cp_sorted_hash *tree, 
							  	   cp_destructor_fn key_dtr,
							  	   cp_destructor_fn val_dtr);

/* insertion function */
CPROPS_DLL
void *cp_sorted_hash_insert(cp_sorted_hash *tree, void *key, void *value);

/* retrieve the value mapped to the given key */
CPROPS_DLL
void *cp_sorted_hash_get(cp_sorted_hash *tree, void *key);

/* find the value of the closest key by operator */
CPROPS_DLL
void *cp_sorted_hash_find(cp_sorted_hash *tree, cp_mapping *mapping, cp_op op);

/* return non-zero if a mapping for 'key' could be found */
CPROPS_DLL
int cp_sorted_hash_contains(cp_sorted_hash *tree, void *key);

/* delete a mapping */
CPROPS_DLL
void *cp_sorted_hash_delete(cp_sorted_hash *tree, void *key);

/* 
 * perform a pre-order iteration over the tree, calling 'callback' on each 
 * node
 */
CPROPS_DLL
int cp_sorted_hash_callback_preorder(cp_sorted_hash *tree, 
									 cp_callback_fn callback, 
									 void *prm);
/* 
 * perform an in-order iteration over the tree, calling 'callback' on each 
 * node
 */
CPROPS_DLL
int cp_sorted_hash_callback(cp_sorted_hash *tree, 
							cp_callback_fn callback, 
							void *prm);
/* 
 * perform a post-order iteration over the tree, calling 'callback' on each 
 * node
 */

CPROPS_DLL
int cp_sorted_hash_callback_postorder(cp_sorted_hash *tree, 
								 	  cp_callback_fn callback, 
									  void *prm);

/* return the number of mappings in the tree */
CPROPS_DLL
int cp_sorted_hash_count(cp_sorted_hash *tree);

/* 
 * lock tree for reading or writing as specified by type parameter. 
 */
CPROPS_DLL
int cp_sorted_hash_lock(cp_sorted_hash *tree, int type);
/* read lock */
#define cp_sorted_hash_rdlock(tree) (cp_sorted_hash_lock((tree), COLLECTION_LOCK_READ))
/* write lock */
#define cp_sorted_hash_wrlock(tree) (cp_sorted_hash_lock((tree), COLLECTION_LOCK_WRITE))
/* unlock */
CPROPS_DLL
int cp_sorted_hash_unlock(cp_sorted_hash *tree);


/* return the table mode indicator */
CPROPS_DLL
int cp_sorted_hash_get_mode(cp_sorted_hash *tree);
/* set mode bits on the tree mode indicator */
CPROPS_DLL
int cp_sorted_hash_set_mode(cp_sorted_hash *tree, int mode);
/* unset mode bits on the tree mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the tree was not previously synchronized, the 
 * internal synchronization structure is initalized.
 */
CPROPS_DLL
int cp_sorted_hash_unset_mode(cp_sorted_hash *tree, int mode);


/** print tree to stdout */
CPROPS_DLL
void cp_sorted_hash_dump(cp_sorted_hash *tree);

__END_DECLS

/** @} */

#endif

