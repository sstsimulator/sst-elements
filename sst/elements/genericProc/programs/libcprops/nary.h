#ifndef _CP_NARYTREE_H
#define _CP_NARYTREE_H

/** @{ */
/**
 * @file
 *
 * cp_narytree is an in-memory btree implementation. 
 */

#include "common.h"

__BEGIN_DECLS

#include "collection.h"

struct _cp_narytree;

typedef CPROPS_DLL struct _cp_narynode
{
	void **key;
	void **value;
	int items;

	struct _cp_narynode **child;
	struct _cp_narynode *parent;
} cp_narynode;

CPROPS_DLL
cp_narynode *cp_narynode_create(void *key, void *value);
CPROPS_DLL
void cp_narynode_destroy(cp_narynode *node);
CPROPS_DLL
void cp_narynode_destroy_deep(cp_narynode *node, struct _cp_narytree *owner);

typedef CPROPS_DLL struct _cp_narytree
{
	cp_narynode *root;

	int items;
	int order;                   /* max. number of children per node */

	int mode;

	cp_compare_fn cmp;
	cp_copy_fn key_copy;         /* key copy function */
	cp_destructor_fn key_dtr;    /* key destructor */
	cp_copy_fn value_copy;       /* value copy function */
	cp_destructor_fn value_dtr;  /* value destructor */

	cp_lock *lock;
	cp_thread txowner;           /* set if a transaction is in progress */
	int txtype;                  /* lock type */
} cp_narytree;

CPROPS_DLL
cp_narytree *cp_narytree_create(int order, cp_compare_fn cmp);

CPROPS_DLL
cp_narytree *cp_narytree_create_by_option(int mode, 
										  int order, 
										  cp_compare_fn cmp, 
										  cp_copy_fn key_copy,
										  cp_destructor_fn key_dtr, 
										  cp_copy_fn value_copy, 
										  cp_destructor_fn value_dtr);

CPROPS_DLL
void cp_narytree_destroy(cp_narytree *tree);

CPROPS_DLL
void cp_narytree_destroy_custom(cp_narytree *tree, 
								cp_destructor_fn key_dtr, 
								cp_destructor_fn value_dtr);

CPROPS_DLL
void *cp_narytree_insert(cp_narytree *tree, void *key, void *value);
CPROPS_DLL
void *cp_narytree_get(cp_narytree *tree, void *key);
CPROPS_DLL
int cp_narytree_contains(cp_narytree *tree, void *key);
CPROPS_DLL
void *cp_narytree_delete(cp_narytree *tree, void *key);
CPROPS_DLL
int cp_narytree_callback(cp_narytree *tree, 
						 cp_callback_fn callback, 
						 void *prm);
CPROPS_DLL
int cp_narytree_count(cp_narytree *tree);

/** print tree to stdout */
CPROPS_DLL
void cp_narytree_dump(cp_narytree *tree);

CPROPS_DLL
int cp_narytree_get_mode(cp_narytree *tree);
	
CPROPS_DLL
int cp_narytree_set_mode(cp_narytree *tree, int mode);

/* unset mode bits on the tree mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the tree was not previously synchronized, the 
 * internal synchronization structure is initialized.
 */
CPROPS_DLL
int cp_narytree_unset_mode(cp_narytree *tree, int mode);


/* lock and set the transaction indicators */
CPROPS_DLL
int cp_narytree_lock(cp_narytree *tree, int type);

#define cp_narytree_rdlock(tree) \
	(cp_narytree_lock((tree), COLLECTION_LOCK_READ))

#define cp_narytree_wrlock(tree) \
	(cp_narytree_lock((tree), COLLECTION_LOCK_WRITE))
		
/* unset the transaction indicators and unlock */
CPROPS_DLL
int cp_narytree_unlock(cp_narytree *tree);

__END_DECLS

/** @} */

#endif

