#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "splay.h"

cp_splaynode *cp_splaynode_create(void *key, void *value, cp_mempool *pool)
{
	cp_splaynode *node;
	if (pool) 
		node = cp_mempool_calloc(pool);
	else
		node = calloc(1, sizeof(cp_splaynode));
	if (node == NULL) return NULL;

	node->key = key;
	node->value = value;

	return node;
}
	
void cp_splaytree_destroy_node(cp_splaytree *tree, cp_splaynode *node)
{
	if (node)
	{
		if ((tree->mode & COLLECTION_MODE_DEEP))
		{
			if (tree->key_dtr) (*tree->key_dtr)(node->key);
			if ((tree->mode & COLLECTION_MODE_MULTIPLE_VALUES) && node->value)
				cp_vector_destroy_custom(node->value, tree->value_dtr);
			else if (tree->value_dtr) 
				(*tree->value_dtr)(node->value);
		}
		else if ((tree->mode & COLLECTION_MODE_MULTIPLE_VALUES) && node->value)
			cp_vector_destroy(node->value);
		if (tree->mempool)
			cp_mempool_free(tree->mempool, node);
		else
			free(node);
	}
}

void cp_splaytree_destroy_node_deep(cp_splaytree *tree, cp_splaynode *node)
{
	if (node)
	{
		cp_splaynode *parent;
		cp_splaynode *child;

		parent = node;
		while (node)
		{
			if (node->right && node->right != parent)
			{
				child = node->right;
				node->right = parent;
				parent = node;
				node = child;
			}
			else if (node->left && node->left != parent)
			{
				child = node->left;
				node->left = node->right;
				node->right = parent;
				parent = node;
				node = child;
			}
			else
			{
				cp_splaytree_destroy_node(tree, node);
				if (node != parent)
				{
					node = parent;
					parent = node->right;
				}
				else
					break;
			}
		}
	}
}

cp_splaytree *cp_splaytree_create(cp_compare_fn cmp)
{
	cp_splaytree *tree = calloc(1, sizeof(cp_splaytree));
	if (tree == NULL) return NULL;

	tree->cmp = cmp;
	tree->mode = COLLECTION_MODE_NOSYNC;

	return tree;
}

/*
 * complete parameter create function
 */
cp_splaytree *
	cp_splaytree_create_by_option(int mode, 
								  cp_compare_fn cmp, 
								  cp_copy_fn key_copy, 
								  cp_destructor_fn key_dtr,
								  cp_copy_fn val_copy, 
								  cp_destructor_fn val_dtr)
{
	cp_splaytree *tree = cp_splaytree_create(cmp);
	if (tree == NULL) return NULL;
	
	tree->mode = mode;
	tree->key_copy = key_copy;
	tree->key_dtr = key_dtr;
	tree->value_copy = val_copy;
	tree->value_dtr = val_dtr;

	if (!(mode & COLLECTION_MODE_NOSYNC))
	{
		tree->lock = malloc(sizeof(cp_lock));
		if (tree->lock == NULL) 
		{
			cp_splaytree_destroy(tree);
			return NULL;
		}
		if (cp_lock_init(tree->lock, NULL) != 0)
		{
			cp_splaytree_destroy(tree);
			return NULL;
		}
	}

	return tree;
}

void cp_splaytree_destroy(cp_splaytree *tree)
{
	if (tree)
	{
		cp_splaytree_destroy_node_deep(tree, tree->root);
		if (tree->lock)
		{
			cp_lock_destroy(tree->lock);
			free(tree->lock);
		}
		free(tree);
	}
}

void cp_splaytree_destroy_custom(cp_splaytree *tree, 
							     cp_destructor_fn key_dtr,
							     cp_destructor_fn val_dtr)
{
	tree->mode |= COLLECTION_MODE_DEEP;
	tree->key_dtr = key_dtr;
	tree->value_dtr = val_dtr;
	cp_splaytree_destroy(tree);
}

static int cp_splaytree_lock_internal(cp_splaytree *tree, int type)
{
    int rc;

    switch (type)
    {
        case COLLECTION_LOCK_READ:
            rc = cp_lock_rdlock(tree->lock);
            break;

        case COLLECTION_LOCK_WRITE:
            rc = cp_lock_wrlock(tree->lock);
            break;

        case COLLECTION_LOCK_NONE:
            rc = 0;
            break;

        default:
            rc = EINVAL;
            break;
    }

	/* api functions may rely on errno to report locking failure */
	if (rc) errno = rc;

    return rc;
}

static int cp_splaytree_unlock_internal(cp_splaytree *tree)
{
	return cp_lock_unlock(tree->lock);
}

int cp_splaytree_txlock(cp_splaytree *tree, int type)
{
	/* clear errno to allow applications to detect locking failure */
	errno = 0;
	
	if (tree->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (tree->mode & COLLECTION_MODE_IN_TRANSACTION && 
		tree->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, tree->txowner)) return 0;
	}
	return cp_splaytree_lock_internal(tree, type);
}

int cp_splaytree_txunlock(cp_splaytree *tree)
{
	if (tree->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (tree->mode & COLLECTION_MODE_IN_TRANSACTION && 
		tree->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, tree->txowner)) return 0;
	}
	return cp_splaytree_unlock_internal(tree);
}

/* lock and set the transaction indicators */
int cp_splaytree_lock(cp_splaytree *tree, int type)
{
	int rc;
	if ((tree->mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	if ((rc = cp_splaytree_lock_internal(tree, type))) return rc;
	tree->txtype = type;
	tree->txowner = cp_thread_self();
	tree->mode |= COLLECTION_MODE_IN_TRANSACTION;
	return 0;
}

/* unset the transaction indicators and unlock */
int cp_splaytree_unlock(cp_splaytree *tree)
{
	cp_thread self = cp_thread_self();
	if (tree->txowner == self)
	{
		tree->txowner = 0;
		tree->txtype = 0;
		tree->mode ^= COLLECTION_MODE_IN_TRANSACTION;
	}
	else if (tree->txtype == COLLECTION_LOCK_WRITE)
		return -1;
	return cp_splaytree_unlock_internal(tree);
}

/* set mode bits on the tree mode indicator */
int cp_splaytree_set_mode(cp_splaytree *tree, int mode)
{
	int rc;
	int nosync; 

	/* can't set NOSYNC in the middle of a transaction */
	if ((tree->mode & COLLECTION_MODE_IN_TRANSACTION) && 
		(mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	/* COLLECTION_MODE_MULTIPLE_VALUES must be set at creation time */	
	if ((mode & COLLECTION_MODE_MULTIPLE_VALUES)) return EINVAL;

	if ((rc = cp_splaytree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
	nosync = tree->mode & COLLECTION_MODE_NOSYNC;

	tree->mode |= mode;

	if (!nosync)
		cp_splaytree_txunlock(tree);

	return 0;
}

/* unset mode bits on the tree mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the tree was not previously synchronized, the 
 * internal synchronization structure is initalized.
 */
int cp_splaytree_unset_mode(cp_splaytree *tree, int mode)
{
	int rc;
	int nosync;

	/* COLLECTION_MODE_MULTIPLE_VALUES can't be unset */
	if ((mode & COLLECTION_MODE_MULTIPLE_VALUES)) return EINVAL;

	if ((rc = cp_splaytree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
	nosync = tree->mode & COLLECTION_MODE_NOSYNC;

	/* handle the special case of unsetting COLLECTION_MODE_NOSYNC */
	if ((mode & COLLECTION_MODE_NOSYNC) && tree->lock == NULL)
	{
		/* tree can't be locked in this case, no need to unlock on failure */
		if ((tree->lock = malloc(sizeof(cp_lock))) == NULL)
			return -1; 
		if (cp_lock_init(tree->lock, NULL))
			return -1;
	}
	
	/* unset specified bits */
    tree->mode &= tree->mode ^ mode;
	if (!nosync)
		cp_splaytree_txunlock(tree);

	return 0;
}

int cp_splaytree_get_mode(cp_splaytree *tree)
{
    return tree->mode;
}


/*  right_rotate - the splay 'zig' operation
 * 
 *       (P)                (Q)
 *      /   \              /   \
 *    (Q)    3    ==>     1    (P)  
 *   /   \                    /   \
 *  1     2                  2     3
 *
 */
void right_rotate(cp_splaynode **node)
{
	cp_splaynode *p = *node;
	cp_splaynode *q = p->left;
	p->left = q->right;
	q->right = p;
	*node = q;
}

/*  left_rotate - the splay 'zag' operation
 * 
 *    (P)                (Q)
 *   /   \              /   \
 *  1    (Q)    ==>   (P)    3
 *      /   \        /   \
 *     2     3      1     2
 *
 */
void left_rotate(cp_splaynode **node)
{
	cp_splaynode *p = *node;
	cp_splaynode *q = p->right;

	p->right = q->left;
	q->left = p;
	*node = q;
}

/*  left-right rotate - the splay 'zig-zag' operation
 * 
 *          (P)                (R)
 *         /   \              /   \
 *      (Q)     4   ==>     (Q)    (P)  
 *      / \                /  \    /  \
 *     1  (R)             1    2  3    4
 *        / \
 *       2   3
 *
 */
static void left_right_rotate(cp_splaynode **node)
{
	cp_splaynode *p = *node;
	cp_splaynode *q = (*node)->left;
	cp_splaynode *r = q->right;

	q->right = r->left;
	p->left = r->right;
	r->right = p;
	r->left = q;

	*node = r;
}

/*  right-left rotate - the splay 'zag-zig' operation
 * 
 *          (P)                   (R)
 *         /   \                 /   \
 *        1     (Q)     ==>    (P)    (Q)  
 *             /  \           /  \    /  \
 *           (R)   4         1    2  3    4
 *           / \
 *          2   3
 *
 */
static void right_left_rotate(cp_splaynode **node)
{
	cp_splaynode *p = *node;
	cp_splaynode *q = (*node)->right;
	cp_splaynode *r = q->left;

	q->left = r->right;
	p->right = r->left;
	r->left = p;
	r->right = q;

	*node = r;
}

/* implement COLLECTION_MODE_COPY, COLLECTION_MODE_MULTIPLE_VALUES if set */
static cp_splaynode *
	create_splaynode(cp_splaytree *tree, void *key, void *value)
{
	cp_splaynode *node = NULL;

	if (tree->mode & COLLECTION_MODE_COPY)
	{
		void *k = tree->key_copy ? (*tree->key_copy)(key) : key;
		if (k)
		{
			void *v = tree->value_copy ? (*tree->value_copy)(value) : value;
			if (v)
				node = cp_splaynode_create(k, v, tree->mempool);
		}
	}
	else
		node = cp_splaynode_create(key, value, tree->mempool);

	if (node && (tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
	{
		cp_vector *m = cp_vector_create(1);
		if (m == NULL) 
		{
			cp_splaytree_destroy_node(tree, node);
			return NULL;
		}

		cp_vector_add_element(m, node->value);
		node->value = m;
	}

	return node;
}

/* update_splaynode - implement COLLECTION_MODE_COPY, COLLECTION_MODE_DEEP and
 * COLLECTION_MODE_MULTIPLE_VALUES when inserting a value for an existing key
 */
static void *
	update_splaynode(cp_splaytree *tree, 
					 cp_splaynode *node, 
					 void *key, void *value)
{
	void *new_key = key;
	void *new_value = value;

	if (tree->mode & COLLECTION_MODE_COPY)
	{
		if (tree->key_copy) 
		{
			new_key = (*tree->key_copy)(key);
			if (new_key == NULL) return NULL;
		}
		if (tree->value_copy)
		{
			new_value = (*tree->value_copy)(value);
			if (new_value == NULL) return NULL;
		}
	}

	if (tree->mode & COLLECTION_MODE_DEEP)
	{
		if (tree->key_dtr)
			(*tree->key_dtr)(node->key);
		if (tree->value_dtr && !(tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
			(*tree->value_dtr)(node->value);
	}
		
	node->key = new_key;
	if ((tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
	{
		cp_vector_add_element(node->value, new_value);
		return node->value;
	}
	else
		node->value = new_value;

	return new_value;
}


/* desplay - the splay balancing act on recursion fold 
 *
 * cp_splaytree divides the implementation of the splay operation between
 * the tree operations, which recursively walk the tree down to the object 
 * node, and the de-splay function, which performs splay rotations on the way
 * out of the recursion
 */
static void desplay(cp_splaynode **node, int *splay_factor, int op)
{
	switch (*splay_factor)
	{
		case 0: 
			*splay_factor += op;
			break;

		case -1:
			if (op == 0)
				right_rotate(node);
			else
			{
				if (op == -1)
				{
					right_rotate(node);
					right_rotate(node);
				}
				else
					right_left_rotate(node);
				*splay_factor = 0;
			}
			break;
			
		case 1:
			if (op == 0)
				left_rotate(node);
			else
			{
				if (op == 1)
				{
					left_rotate(node);
					left_rotate(node);
				}
				else
					left_right_rotate(node);
				*splay_factor = 0;
			}
			break;
	}
}

static void *
	splay_insert(cp_splaytree *tree, 
			     cp_splaynode **node, 
				 void *key, void *value,
				 int *splay_factor)
{
	int cmp;
	void *res;
	
	if (*node == NULL)
	{
		*node = create_splaynode(tree, key, value);
		if (*node) 
		{
			tree->items++;
			return (*node)->value;
		}
		return NULL;
	}

	res = (*node)->value;
		
	cmp = tree->cmp((*node)->key, key);
	if (cmp == 0)	/* replace */
	{
		*splay_factor = 0;
		return update_splaynode(tree, *node, key, value);
	}
	else if (cmp > 0) /* go left */
	{
		res = splay_insert(tree, &(*node)->left, key, value, splay_factor);
		desplay(node, splay_factor, -1);
	}
	else /* go right*/
	{
		res = splay_insert(tree, &(*node)->right, key, value, splay_factor);
		desplay(node, splay_factor, 1);
	}
	
	return res;
}

void *cp_splaytree_insert(cp_splaytree *tree, void *key, void *value)
{
	void *res;
	int splay_factor = 0;
	
	/* lock unless COLLECTION_MODE_NOSYNC is set or this thread owns the tx */
	if (cp_splaytree_txlock(tree, COLLECTION_LOCK_WRITE)) return NULL;
	
	res = splay_insert(tree, &tree->root, key, value, &splay_factor);
	desplay(&tree->root, &splay_factor, 0);

	/* unlock unless COLLECTION_MODE_NOSYNC set or this thread owns the tx */
	cp_splaytree_txunlock(tree);
	
	return res;
}

static void *
	splay_get(cp_splaytree *tree, 
			  cp_splaynode **node, 
			  void *key, 
			  int *splay_factor)
{
	int cmp;
	void *res;
	
	if (*node == NULL) return NULL;

	cmp = tree->cmp((*node)->key, key);
	if (cmp == 0)	/* found */
	{
		*splay_factor = 0;
		res = (*node)->value;
	}
	else if (cmp > 0) /* go left */
	{
		if ((res = splay_get(tree, &(*node)->left, key, splay_factor)))
			desplay(node, splay_factor, -1);
	}
	else /* go right*/
	{
		if ((res = splay_get(tree, &(*node)->right, key, splay_factor)))
			desplay(node, splay_factor, 1);
	}
	
	return res;
}

void *cp_splaytree_get(cp_splaytree *tree, void *key)
{
	int splay_factor = 0;
	cp_splaynode **node = &tree->root;
	void *res = NULL;

	/* the search operation on a splay tree brings the requested mapping to
	 * the root of the tree - this being the point to the splay tree data
	 * structure. Since the tree structure may change, the tree must be locked
	 * for writing here (unless COLLECTION_MODE_NOSYNC is set).
	 */
	if (cp_splaytree_txlock(tree, COLLECTION_LOCK_WRITE)) return NULL;

	res = splay_get(tree, node, key, &splay_factor);
	desplay(node, &splay_factor, 0);

	/* unlock unless COLLECTION_MODE_NOSYNC set or this thread owns the tx */
	cp_splaytree_txunlock(tree);

	return res;
}
	
static void *
	splaytree_find_internal(cp_splaytree *tree, 
						    cp_splaynode *curr, 
						  	void *key, 
						  	cp_op op)
{
	void *res = NULL;

	if (curr)
	{
		int cmp = tree->cmp(key, curr->key);
		if (cmp == 0)
		{
			if (op == CP_OP_NE)
			{
				if (curr->right) return curr->right->value;
				if (curr->left) return curr->left->value;
			}

			if (op == CP_OP_GT)
			{
				if (curr->right)
				{
					curr = curr->right;
					while (curr->left) curr = curr->left;
				}
				else
					return NULL;
			}

			if (op == CP_OP_LT)
			{
				if (curr->left)
				{
					curr = curr->left;
					while (curr->right) curr = curr->right;
				}
				else
					return NULL;
			}

			return curr->value;
		}
		else
		{
			if (op == CP_OP_NE) return curr->value;
			if (cmp > 0) 
				res = splaytree_find_internal(tree, curr->right, key, op);
			else
				res = splaytree_find_internal(tree, curr->left, key, op);

			if (res == NULL)
			{
				if ((cmp > 0 && (op == CP_OP_LT || op == CP_OP_LE)) ||
				    (cmp < 0 && (op == CP_OP_GE || op == CP_OP_GT)))
						res = curr->value;
			}
		}
	}

	return res;
}

void *cp_splaytree_find(cp_splaytree *tree, void *key, cp_op op)
{
	void *res = NULL;
	if (op == CP_OP_EQ) return cp_splaytree_get(tree, key);

	/* lock unless COLLECTION_MODE_NOSYNC is set or this thread owns the tx */
	if (cp_splaytree_txlock(tree, COLLECTION_LOCK_READ)) return NULL;

	res = splaytree_find_internal(tree, tree->root, key, op);

	/* unlock unless COLLECTION_MODE_NOSYNC set or this thread owns the tx */
	cp_splaytree_txunlock(tree);

	return res;
}

int cp_splaytree_contains(cp_splaytree *tree, void *key)
{
	return (cp_splaytree_get(tree, key) != NULL);
}

/* helper function for deletion */
static void swap_node_content(cp_splaynode *a, cp_splaynode *b)
{
	void *tmpkey, *tmpval;

	tmpkey = a->key;
	a->key = b->key;
	b->key = tmpkey;
	
	tmpval = a->value;
	a->value = b->value;
	b->value = tmpval;
}

static void *
	splay_delete(cp_splaytree *tree, 
				 cp_splaynode **node,
				 void *key, 
				 int *splay_factor)
{
	void *res;
	int cmp;
	
	if ((*node) == NULL) 
	{
		*splay_factor = 0;
		return NULL;
	}
	
	cmp = (*tree->cmp)((*node)->key, key);
	if (cmp == 0)
	{
		cp_splaynode *x;
		res = (*node)->value;
		tree->items--;
		
		if ((*node)->right && (*node)->left)
		{
			cp_splaynode *surrogate = *node;
			node = &(*node)->right;
			while ((*node)->left) node = &(*node)->left;
			swap_node_content(*node, surrogate);
		}

		x = *node;
		if ((*node)->right)
			*node = (*node)->right;
		else
			*node = (*node)->left;
		cp_splaytree_destroy_node(tree, x);
		
		*splay_factor = 0;
	}
	else if (cmp > 0)
	{
		res = splay_delete(tree, &(*node)->left, key, splay_factor);
		desplay(node, splay_factor, -1);
	}
	else
	{
		res = splay_delete(tree, &(*node)->right, key, splay_factor);
		desplay(node, splay_factor, 1);
	}

	return res;
}
				 
void *cp_splaytree_delete(cp_splaytree *tree, void *key)
{
	void *res = NULL;
	int splay_factor = 0;
	cp_splaynode **node;

	if (cp_splaytree_txlock(tree, COLLECTION_LOCK_WRITE)) return NULL;
	
	node = &tree->root;

	res = splay_delete(tree, node, key, &splay_factor);
	desplay(node, &splay_factor, 0);

	cp_splaytree_txunlock(tree);
	
	return res;
}


static int 
	splay_scan_pre_order(cp_splaynode *node, 
						 cp_callback_fn callback, 
						 void *prm)
{
	int rc;
	
	if (node) 
	{
		if ((rc = (*callback)(node, prm))) return rc;
		if ((rc = splay_scan_pre_order(node->right, callback, prm))) return rc;
		if ((rc = splay_scan_pre_order(node->left, callback, prm))) return rc;
	}

	return 0;
}

int cp_splaytree_callback_preorder(cp_splaytree *tree, 
								   cp_callback_fn callback, 
								   void *prm)
{
	int rc;

	if ((rc = cp_splaytree_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = splay_scan_pre_order(tree->root, callback, prm);
	cp_splaytree_txunlock(tree);

	return rc;
}

static int 
	splay_scan_in_order(cp_splaynode *node, cp_callback_fn callback, void *prm)
{
	int rc;
	
	if (node) 
	{
		if ((rc = splay_scan_in_order(node->right, callback, prm))) return rc;
		if ((rc = (*callback)(node, prm))) return rc;
		if ((rc = splay_scan_in_order(node->left, callback, prm))) return rc;
	}

	return 0;
}

int cp_splaytree_callback(cp_splaytree *tree, 
						  cp_callback_fn callback, 
						  void *prm)
{
	int rc;

	if ((rc = cp_splaytree_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = splay_scan_in_order(tree->root, callback, prm);
	cp_splaytree_txunlock(tree);

	return rc;
}

static int 
	splay_scan_post_order(cp_splaynode *node, 
						  cp_callback_fn callback, 
						  void *prm)
{
	int rc;
	
	if (node) 
	{
		if ((rc = splay_scan_post_order(node->right, callback, prm))) return rc;
		if ((rc = splay_scan_post_order(node->left, callback, prm))) return rc;
		if ((rc = (*callback)(node, prm))) return rc;
	}

	return 0;
}

int cp_splaytree_callback_postorder(cp_splaytree *tree, 
									cp_callback_fn callback, 
									void *prm)
{
	int rc;

	if ((rc = cp_splaytree_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = splay_scan_post_order(tree->root, callback, prm);
	cp_splaytree_txunlock(tree);

	return rc;
}

int cp_splaytree_count(cp_splaytree *tree)
{
	return tree->items;
}


void cp_splaynode_print(cp_splaynode *node, int level)
{
	int i;
	if (node->right) cp_splaynode_print(node->right, level + 1);
	for (i = 0; i < level; i++) printf("  . ");
	printf("[%s => %s]\n", (char *) node->key, (char *) node->value);
	if (node->left) cp_splaynode_print(node->left, level + 1);
}

void cp_splaynode_multi_print(cp_splaynode *node, int level)
{
	int i;
	cp_vector *v = node->value;
	if (node->right) cp_splaynode_multi_print(node->right, level + 1);
	
	for (i = 0; i < level; i++) printf("  . ");
	printf("[%s => ", (char *) node->key);

	for (i = 0; i < cp_vector_size(v); i++)
		printf("%s; ", (char *) cp_vector_element_at(v, i));

	printf("]\n");

	if (node->left) cp_splaynode_multi_print(node->left, level + 1);
}

void cp_splaytree_dump(cp_splaytree *tree)
{
	if (tree->root) 
	{
		if ((tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
			cp_splaynode_multi_print(tree->root, 0);
		else
			cp_splaynode_print(tree->root, 0);
	}
}

/* set tree to use given mempool or allocate a new one if pool is NULL */
int cp_splaytree_use_mempool(cp_splaytree *tree, cp_mempool *pool)
{
	int rc = 0;
	
	if ((rc = cp_splaytree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
	if (pool)
	{
		if (pool->item_size < sizeof(cp_splaynode))
		{
			rc = EINVAL;
			goto DONE;
		}
		if (tree->mempool) 
		{
			if (tree->items) 
			{
				rc = ENOTEMPTY;
				goto DONE;
			}
			cp_mempool_destroy(tree->mempool);
		}
		cp_mempool_inc_refcount(pool);
		tree->mempool = pool;
	}
	else
	{
		tree->mempool = 
			cp_mempool_create_by_option(COLLECTION_MODE_NOSYNC, 
										sizeof(cp_splaynode), 0);
		if (tree->mempool == NULL) 
		{
			rc = ENOMEM;
			goto DONE;
		}
	}

DONE:
	cp_splaytree_txunlock(tree);
	return rc;
}


/* set tree to use a shared memory pool */
int cp_splaytree_share_mempool(cp_splaytree *tree, cp_shared_mempool *pool)
{
	int rc;

	if ((rc = cp_splaytree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;

	if (tree->mempool)
	{
		if (tree->items)
		{
			rc = ENOTEMPTY;
			goto DONE;
		}

		cp_mempool_destroy(tree->mempool);
	}

	tree->mempool = cp_shared_mempool_register(pool, sizeof(cp_splaynode));
	if (tree->mempool == NULL) 
	{
		rc = ENOMEM;
		goto DONE;
	}
	
DONE:
	cp_splaytree_txunlock(tree);
	return rc;
}

