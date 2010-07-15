#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "collection.h"
#include "avl.h"

cp_avlnode *cp_avlnode_create(void *key, void *value, cp_mempool *mempool)
{
	cp_avlnode *node;
	if (mempool)
		node = (cp_avlnode *) cp_mempool_calloc(mempool);
	else
		node = (cp_avlnode *) calloc(1, sizeof(cp_avlnode));
	if (node == NULL) return NULL;

	node->key = key;
	node->value = value;

	return node;
}
	
void cp_avltree_destroy_node(cp_avltree *tree, cp_avlnode *node)
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

void cp_avltree_destroy_node_deep(cp_avltree *tree, cp_avlnode *node)
{
	if (node)
  	{
  		cp_avlnode *parent = node;
		cp_avlnode *child;

		while (1)
		{
			if (node->right)
			{
				child = node->right;
				node->right = node->left;
				node->left = parent;
				parent = node;
				node = child;
			}
			else
			{
				if (node->left)
					cp_avltree_destroy_node(tree, node->left);

				cp_avltree_destroy_node(tree, node);
				if (node != parent)
				{
					node = parent;
					parent = node->left;
					node->left = NULL;
				}
				else
					break;
			}
		}
	}
}

/* default create function - default mode is COLLECTION_MODE_NOSYNC */
cp_avltree *cp_avltree_create(cp_compare_fn cmp)
{
	cp_avltree *tree = calloc(1, sizeof(cp_avltree));
	if (tree == NULL) return NULL;

	tree->mode = COLLECTION_MODE_NOSYNC;
	tree->cmp = cmp;

	return tree;
}

/*
 * complete parameter create function
 */
cp_avltree *
	cp_avltree_create_by_option(int mode, cp_compare_fn cmp, 
								cp_copy_fn key_copy, cp_destructor_fn key_dtr,
								cp_copy_fn val_copy, cp_destructor_fn val_dtr)
{
	cp_avltree *tree = cp_avltree_create(cmp);
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
			cp_avltree_destroy(tree);
			return NULL;
		}
		if (cp_lock_init(tree->lock, NULL) != 0)
		{
			cp_avltree_destroy(tree);
			return NULL;
		}
	}

	return tree;
}

void cp_avltree_destroy(cp_avltree *tree)
{
	if (tree)
	{
		cp_avltree_destroy_node_deep(tree, tree->root);
		if (tree->lock)
		{
			cp_lock_destroy(tree->lock);
			free(tree->lock);
		}
		free(tree);
	}
}

void cp_avltree_destroy_custom(cp_avltree *tree, 
							   cp_destructor_fn key_dtr,
							   cp_destructor_fn val_dtr)
{
	tree->mode |= COLLECTION_MODE_DEEP;
	tree->key_dtr = key_dtr;
	tree->value_dtr = val_dtr;
	cp_avltree_destroy(tree);
}


static int cp_avltree_lock_internal(cp_avltree *tree, int type)
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

static int cp_avltree_unlock_internal(cp_avltree *tree)
{
	return cp_lock_unlock(tree->lock);
}

int cp_avltree_txlock(cp_avltree *tree, int type)
{
	/* clear errno to allow client code to distinguish between a NULL return
	 * value indicating the tree doesn't contain the requested value and NULL
	 * on locking failure in tree operations
	 */
	errno = 0;
	if (tree->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (tree->mode & COLLECTION_MODE_IN_TRANSACTION && 
		tree->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, tree->transaction_owner)) return 0;
	}
	return cp_avltree_lock_internal(tree, type);
}

int cp_avltree_txunlock(cp_avltree *tree)
{
	if (tree->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (tree->mode & COLLECTION_MODE_IN_TRANSACTION && 
		tree->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, tree->transaction_owner)) return 0;
	}
	return cp_avltree_unlock_internal(tree);
}

/* lock and set the transaction indicators */
int cp_avltree_lock(cp_avltree *tree, int type)
{
	int rc;
	if ((tree->mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	if ((rc = cp_avltree_lock_internal(tree, type))) return rc;
	tree->txtype = type;
	tree->transaction_owner = cp_thread_self();
	tree->mode |= COLLECTION_MODE_IN_TRANSACTION;
	return 0;
}

/* unset the transaction indicators and unlock */
int cp_avltree_unlock(cp_avltree *tree)
{
	cp_thread self = cp_thread_self();
	if (tree->transaction_owner == self)
	{
		tree->transaction_owner = 0;
		tree->txtype = 0;
		tree->mode ^= COLLECTION_MODE_IN_TRANSACTION;
	}
	else if (tree->txtype == COLLECTION_LOCK_WRITE)
		return CP_UNLOCK_FAILED;
	return cp_avltree_unlock_internal(tree);
}

/* set mode bits on the tree mode indicator */
int cp_avltree_set_mode(cp_avltree *tree, int mode)
{
	int rc;
	int nosync; 

	/* COLLECTION_MODE_MULTIPLE_VALUES must be set at creation time */
	if ((mode & COLLECTION_MODE_MULTIPLE_VALUES)) return EINVAL;
	/* can't set NOSYNC in the middle of a transaction */
	if ((tree->mode & COLLECTION_MODE_IN_TRANSACTION) && 
		(mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	
	if ((rc = cp_avltree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
	nosync = tree->mode & COLLECTION_MODE_NOSYNC;

	tree->mode |= mode;

	if (!nosync)
		 rc = cp_avltree_txunlock(tree);

	return rc;
}

/* unset mode bits on the tree mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the tree was not previously synchronized, the 
 * internal synchronization structure is initalized.
 */
int cp_avltree_unset_mode(cp_avltree *tree, int mode)
{
	int rc;
	int nosync;

	/* COLLECTION_MODE_MULTIPLE_VALUES can't be unset */
	if ((mode & COLLECTION_MODE_MULTIPLE_VALUES)) return EINVAL;

	nosync = tree->mode & COLLECTION_MODE_NOSYNC;
	if ((rc = cp_avltree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
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
		cp_avltree_txunlock(tree);

	return 0;
}

int cp_avltree_get_mode(cp_avltree *tree)
{
    return tree->mode;
}

/* set tree to use given mempool or allocate a new one if pool is NULL */
int cp_avltree_use_mempool(cp_avltree *tree, cp_mempool *pool)
{
	int rc = 0;
	
	if ((rc = cp_avltree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
	if (pool)
	{
		if (pool->item_size < sizeof(cp_avlnode))
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
										sizeof(cp_avlnode), 0);
		if (tree->mempool == NULL) 
		{
			rc = ENOMEM;
			goto DONE;
		}
	}

DONE:
	cp_avltree_txunlock(tree);
	return rc;
}


/* set tree to use a shared memory pool */
int cp_avltree_share_mempool(cp_avltree *tree, cp_shared_mempool *pool)
{
	int rc;

	if ((rc = cp_avltree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;

	if (tree->mempool)
	{
		if (tree->items)
		{
			rc = ENOTEMPTY;
			goto DONE;
		}

		cp_mempool_destroy(tree->mempool);
	}

	tree->mempool = cp_shared_mempool_register(pool, sizeof(cp_avlnode));
	if (tree->mempool == NULL) 
	{
		rc = ENOMEM;
		goto DONE;
	}
	
DONE:
	cp_avltree_txunlock(tree);
	return rc;
}


#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))									  

/*         left rotate
 *
 *    (P)                (Q)
 *   /   \              /   \
 *  1    (Q)    ==>   (P)    3
 *      /   \        /   \
 *     2     3      1     2
 *
 */
static void left_rotate(cp_avlnode **node)
{
	cp_avlnode *p = *node;
	cp_avlnode *q = p->right;

	p->right = q->left;
	q->left = p;
	*node = q;

	p->balance -= 1 + MAX(0, q->balance);
	q->balance -= 1 - MIN(0, p->balance);
}

/*           right rotate
 *  
 *       (P)                (Q)
 *      /   \              /   \
 *    (Q)    3    ==>     1    (P)  
 *   /   \                    /   \
 *  1     2                  2     3
 *
 */
static void right_rotate(cp_avlnode **node)
{
	cp_avlnode *p = *node;
	cp_avlnode *q = p->left;
	p->left = q->right;
	q->right = p;
	*node = q;

	p->balance += 1 - MIN(0, q->balance);
	q->balance += 1 + MAX(0, p->balance);
}


/*            left-right rotate
 * 
 *          (P)                (R)
 *         /   \              /   \
 *      (Q)     4   ==>     (Q)    (P)  
 *      / \                /  \    /  \
 *     1  (R)             1    2  3    4
 *        / \
 *       2   3
 *
 *  note that left-right rotate could be implemented as a call to 
 *  left_rotate(Q) followed by a call to right_rotate(P).
 */
static void left_right_rotate(cp_avlnode **node)
{
	cp_avlnode *p = *node;
	cp_avlnode *q = (*node)->left;
	cp_avlnode *r = q->right;

	q->right = r->left;
	p->left = r->right;
	r->right = p;
	r->left = q;

	*node = r;

	q->balance -= 1 + MAX(0, r->balance);
	p->balance += 1 - MIN(MIN(0, r->balance) - 1, r->balance + q->balance);
	r->balance += MAX(0, p->balance) + MIN(0, q->balance);
}

/*              right-left rotate
 * 
 *          (P)                   (R)
 *         /   \                 /   \
 *        1     (Q)     ==>    (P)    (Q)  
 *             /  \           /  \    /  \
 *           (R)   4         1    2  3    4
 *           / \
 *          2   3
 *
 *  note that right-left rotate could be implemented as a call to 
 *  right_rotate(Q) followed by a call to left_rotate(P).
 */
static void right_left_rotate(cp_avlnode **node)
{
	cp_avlnode *p = *node;
	cp_avlnode *q = (*node)->right;
	cp_avlnode *r = q->left;

	q->left = r->right;
	p->right = r->left;
	r->left = p;
	r->right = q;

	*node = r;

	q->balance += 1 - MIN(0, r->balance);
	p->balance -= 1 + MAX(MAX(0, r->balance) + 1, r->balance + q->balance);
	r->balance += MAX(0, q->balance) + MIN(0, p->balance);
}

/*
 *  check balances and rotate as required. 
 */
static void rebalance(cp_avlnode **node)
{
	if ((*node)->balance == -2)
	{
		if ((*node)->left->balance == 1)
			left_right_rotate(node);
		else
			right_rotate(node);
	}
	else if ((*node)->balance == 2)
	{
		if ((*node)->right->balance == -1)
			right_left_rotate(node);
		else
			left_rotate(node);
	}
}

/* implement COLLECTION_MODE_COPY if set */
static cp_avlnode *create_avlnode(cp_avltree *tree, void *key, void *value)
{
	cp_avlnode *node = NULL;

	if (tree->mode & COLLECTION_MODE_COPY)
	{
		void *k = tree->key_copy ? (*tree->key_copy)(key) : key;
		if (k)
		{
			void *v = tree->value_copy ? (*tree->value_copy)(value) : value;
			if (v)
				node = cp_avlnode_create(k, v, tree->mempool);
		}
	}
	else
		node = cp_avlnode_create(key, value, tree->mempool);

	if (node && (tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
	{
		cp_vector *m = cp_vector_create(1);
		if (m == NULL) 
		{
			cp_avltree_destroy_node(tree, node);
			return NULL;
		}

		cp_vector_add_element(m, node->value);
		node->value = m;
	}

	return node;
}

/* update_avlnode - implement COLLECTION_MODE_COPY, COLLECTION_MODE_DEEP and
 * COLLECTION_MODE_MULTIPLE_VALUES when inserting a value for an existing key
 */
static void *
	update_avlnode(cp_avltree *tree, cp_avlnode *node, void *key, void *value)
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

static void *avl_insert(cp_avltree *tree, 	      /* the tree */
		      		  	cp_avlnode **node, 	      /* current node */
					  	cp_avlnode *parent,       /* parent node */
					  	void *key, void *value)   /* new entry */
{
	void *res = NULL;
	int before = (*node)->balance;
	int cmp = tree->cmp((*node)->key, key);

	if (cmp == 0)
		return update_avlnode(tree, *node, key, value);

	if (cmp > 0)
	{ 	/* go left */
		if ((*node)->left)
			res = avl_insert(tree, &(*node)->left, *node, key, value);
		else
		{
			(*node)->left = create_avlnode(tree, key, value);
			if ((*node)->left == NULL) return NULL;
			res = (*node)->left->value;
			(*node)->balance--;
			tree->items++;
		}
	}
	else			/* go right */
	{
		if ((*node)->right)
			res = avl_insert(tree, &(*node)->right, *node, key, value);
		else
		{
			(*node)->right = create_avlnode(tree, key, value);
			if ((*node)->right == NULL) return NULL;
			res = (*node)->right->value;
			(*node)->balance++;
			tree->items++;
		}
	}
	
	if (parent && (*node)->balance && before == 0)
		parent->balance += (parent->left == *node ? (-1) : (+1));

	rebalance(node);

	return res;
}

/* cp_avltree_insert recurses through the tree, finds where the new node fits
 * in, puts it there, and balances the tree on the way out of the recursion.
 *
 * if the insertion is successful, the actual inserted value is returned - this
 * may be different than the 'value' parameter if COLLECTION_MODE_COPY is set.
 * On failure cp_avltree_insert returns NULL. insertion may fail if memory for
 * the new node could not be allocated, or if the operation is synchronized 
 * (ie, COLLECTION_MODE_NOSYNC is not set) and locking failed.
 *
 * the current implementation is recursive. An iterative implementation would
 * still require a stack to maintain balance after an insertion, but could save 
 * a few comparisons in determining whether rebalancing is required. The number
 * of saved comparisons would be on the order of log(N/2) on average. For a 
 * tree on the order of million keys, this would mean saving about 10 integer 
 * comparisons. This may be optimized in a future version.
 *
 * Note that cp_avltree replaces the mapping for existing keys by default. To 
 * create multiple mappings for the same key, COLLECTION_MODE_MULTIPLE_VALUES
 * must be set.
 */
void *cp_avltree_insert(cp_avltree *tree, void *key, void *value)
{
	void *res = NULL;

	/* lock unless COLLECTION_MODE_NOSYNC is set or this thread owns the tx */
	if (cp_avltree_txlock(tree, COLLECTION_LOCK_WRITE)) return NULL;
	
	/* perform insert */
	if (tree->root)
	{
		res = avl_insert(tree, &tree->root, NULL, key, value);
	}
	else
	{
		tree->root = create_avlnode(tree, key, value);
		if (tree->root)
		{
			tree->items++;
			res = tree->root->value;
		}
	}

	/* unlock unless COLLECTION_MODE_NOSYNC set or this thread owns the tx */
	cp_avltree_txunlock(tree);
	
	return res;
}

/*
 * cp_avltree_get is an iterative search function, returning the value for the
 * given key if found or NULL otherwise. cp_avltree_get may also return NULL if
 * the operation is synchronized (ie COLLECTION_MODE_NOSYNC is not set) and
 * locking failed. In this case, errno should be set.
 * 
 * Note that if COLLECTION_MODE_MULTIPLE_VALUES is set, the return value is a
 * vector containing the values for the given key. This vector may contain a 
 * single value.
 */
void *cp_avltree_get(cp_avltree *tree, void *key)
{
	void *res = NULL;
	cp_avlnode *curr;
	
	/* lock unless COLLECTION_MODE_NOSYNC is set or this thread owns the tx */
	if (cp_avltree_txlock(tree, COLLECTION_LOCK_READ)) return NULL;
	
	curr = tree->root;

	while (curr)
	{
		int c = tree->cmp(curr->key, key);
		if (c == 0) break;
		curr = (c > 0) ? curr->left : curr ->right;
	}

	if (curr) res = curr->value;

	/* unlock unless COLLECTION_MODE_NOSYNC set or this thread owns the tx */
	cp_avltree_txunlock(tree);

	return res;
}

static void *
	avltree_find_internal(cp_avltree *tree, 
						  cp_avlnode *curr, 
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
				res = avltree_find_internal(tree, curr->right, key, op);
			else
				res = avltree_find_internal(tree, curr->left, key, op);

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

void *cp_avltree_find(cp_avltree *tree, void *key, cp_op op)
{
	void *res = NULL;
	if (op == CP_OP_EQ) return cp_avltree_get(tree, key);

	/* lock unless COLLECTION_MODE_NOSYNC is set or this thread owns the tx */
	if (cp_avltree_txlock(tree, COLLECTION_LOCK_READ)) return NULL;

	res = avltree_find_internal(tree, tree->root, key, op);

	/* unlock unless COLLECTION_MODE_NOSYNC set or this thread owns the tx */
	cp_avltree_txunlock(tree);

	return res;
}

int cp_avltree_contains(cp_avltree *tree, void *key)
{
	return (cp_avltree_get(tree, key) != NULL);
}

/* helper function for step_delete_right, step_delete_left */
static void swap_node_content(cp_avlnode *a, cp_avlnode *b)
{
	void *tmpkey, *tmpval;

	tmpkey = a->key;
	a->key = b->key;
	b->key = tmpkey;
	
	tmpval = a->value;
	a->value = b->value;
	b->value = tmpval;
}

/*
 * target can't be deleted directly because it has no NULL branches, so find 
 * surrogate that doesn't and delete that instead
 */
static cp_avlnode * 
	step_delete_right(cp_avlnode *target, 
					  cp_avlnode **surrogate, 
					  cp_avlnode **prev, 
					  int *balance)
{
	cp_avlnode *rm;
	cp_avlnode **pprev;
	if ((*surrogate)->right == NULL)
	{
		/* keep old key and value on the node being unlinked so they can be
		 * deallocated if COLLECTION_MODE_DEEP is set
		 */
		rm = *surrogate;
		swap_node_content(target, rm);
		*surrogate = (*surrogate)->left;
		*balance = 1;
	}
	else
	{
		pprev = prev;
		prev = surrogate;
		rm = step_delete_right(target, &(*surrogate)->right, prev, balance);
		prev = pprev;
	}
		
	if (*balance)
	{
		if ((*prev) == target)
			(*prev)->balance++;
		else
			(*prev)->balance--;

		rebalance(prev);

		if ((*prev)->balance == 1 || (*prev)->balance == -1)
			*balance = 0;
	}

	return rm;
}

static cp_avlnode * 
	step_delete_left(cp_avlnode *target, 
					 cp_avlnode **surrogate, 
					 cp_avlnode **prev, 
					 int *balance)
{
	cp_avlnode *rm;
	cp_avlnode **pprev;
	if ((*surrogate)->left == NULL)
	{
		/* keep old key and value on the node being unlinked so they can be
		 * deallocated if COLLECTION_MODE_DEEP is set
		 */
		rm = *surrogate;
		swap_node_content(target, rm);
		*surrogate = (*surrogate)->right;
		*balance = 1;
	}
	else
	{
		pprev = prev;
		prev = surrogate;
		rm = step_delete_left(target, &(*surrogate)->left, prev, balance);
		prev = pprev;
	}
		
	if (*balance)
	{
		if ((*prev) == target)
			(*prev)->balance--;
		else
			(*prev)->balance++;

		rebalance(prev);

		if ((*prev)->balance == 1 || (*prev)->balance == -1)
			*balance = 0;
	}

	return rm;
}

static void *avl_delete(cp_avltree *tree, 
						cp_avlnode **node, 
						void *key, int *balance)
{
	void *res = NULL;
	int cmp;
	int direction = 0;

	if (*node == NULL) return NULL;

	cmp = tree->cmp((*node)->key, key);
	if (cmp > 0)
	{
		direction = 1;
		res = avl_delete(tree, &(*node)->left, key, balance);
	}
	else if (cmp < 0)
	{
		direction = -1;
		res = avl_delete(tree, &(*node)->right, key, balance);
	}
	else /* cmp == 0 - found delete candidate, remove it */
	{
		cp_avlnode *old;
		res = (*node)->value; /* store the value to return */

		tree->items--;
		if ((*node)->left == NULL) 
		{
			old = *node;
			*node = (*node)->right;
			*balance = 1;
		}
		else if ((*node)->right == NULL)
		{
			old = *node;
			*node = (*node)->left;
			*balance = 1;
		}
		else
		{
			cp_avlnode **d;
			/* 
			 * although not strictly necessary, test may save some 
			 * rebalancing operations.  
			 */
			if ((*node)->balance > 0)
			{
				d = &(*node)->right;
				old = step_delete_left(*node, d, node, balance);
			}
			else 
			{
				d = &(*node)->left;
				old = step_delete_right(*node, d, node, balance);
			}
			
			if ((*node)->balance == 1 || (*node)->balance == -1) *balance = 0;
		}
		cp_avltree_destroy_node(tree, old);
		return res;
	}

	if (*balance)
		(*node)->balance += direction;

	rebalance(node);

	if ((*node)->balance)
		*balance = 0;

	return res;
}


/* cp_avltree_delete is a recursive mapping removal function, returning the 
 * value mapped to the key removed or NULL if no mapping for the given key is
 * found. cp_avltree_delete may also return NULL if the operation is 
 * synchronized (ie COLLECTION_MODE_NOSYNC is not set) and locking failed. In 
 * this case, errno should be set. Note that the return value may point to 
 * unallocated memory if COLLECTION_MODE_DEEP is set. 
 */
void *cp_avltree_delete(cp_avltree *tree, void *key)
{
	int b = 0;
	void *res = NULL;

	if (cp_avltree_txlock(tree, COLLECTION_LOCK_WRITE)) return NULL;

	res = avl_delete(tree, &tree->root, key, &b);

	cp_avltree_txunlock(tree);

	return res;
}

static int 
	avl_scan_pre_order(cp_avlnode *node, cp_callback_fn callback, void *prm)
{
	int rc;
	
	if (node) 
	{
		if ((rc = (*callback)(node, prm))) return rc;
		if ((rc = avl_scan_pre_order(node->left, callback, prm))) return rc;
		if ((rc = avl_scan_pre_order(node->right, callback, prm))) return rc;
	}

	return 0;
}

int cp_avltree_callback_preorder(cp_avltree *tree, 
								 cp_callback_fn callback, 
								 void *prm)
{
	int rc;

	if ((rc = cp_avltree_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = avl_scan_pre_order(tree->root, callback, prm);
	cp_avltree_txunlock(tree);

	return rc;
}

static int 
	avl_scan_in_order(cp_avlnode *node, cp_callback_fn callback, void *prm)
{
	int rc;
	
	if (node) 
	{
		if ((rc = avl_scan_in_order(node->left, callback, prm))) return rc;
		if ((rc = (*callback)(node, prm))) return rc;
		if ((rc = avl_scan_in_order(node->right, callback, prm))) return rc;
	}

	return 0;
}

int cp_avltree_callback(cp_avltree *tree, cp_callback_fn callback, void *prm)
{
	int rc;

	if ((rc = cp_avltree_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = avl_scan_in_order(tree->root, callback, prm);
	cp_avltree_txunlock(tree);

	return rc;
}

static int 
	avl_scan_post_order(cp_avlnode *node, cp_callback_fn callback, void *prm)
{
	int rc;
	
	if (node) 
	{
		if ((rc = avl_scan_post_order(node->left, callback, prm))) return rc;
		if ((rc = avl_scan_post_order(node->right, callback, prm))) return rc;
		if ((rc = (*callback)(node, prm))) return rc;
	}

	return 0;
}

int cp_avltree_callback_postorder(cp_avltree *tree, 
								  cp_callback_fn callback, 
								  void *prm)
{
	int rc;

	if ((rc = cp_avltree_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = avl_scan_post_order(tree->root, callback, prm);
	cp_avltree_txunlock(tree);

	return rc;
}

int cp_avltree_count(cp_avltree *tree)
{
	return tree->items;
}


void cp_avlnode_print(cp_avlnode *node, int level)
{
	int i;
	if (node->right) cp_avlnode_print(node->right, level + 1);
	for (i = 0; i < level; i++) printf("  . ");
	printf("(%d) [%s => %s]\n", node->balance, (char *) node->key, (char *) node->value);
	if (node->left) cp_avlnode_print(node->left, level + 1);
}

void cp_avlnode_multi_print(cp_avlnode *node, int level)
{
	int i;
	cp_vector *v = node->value;
	if (node->right) cp_avlnode_multi_print(node->right, level + 1);
	
	for (i = 0; i < level; i++) printf("  . ");
	printf("(%d) [%s => ", node->balance, (char *) node->key);

	for (i = 0; i < cp_vector_size(v); i++)
		printf("%s; ", (char *) cp_vector_element_at(v, i));

	printf("]\n");

	if (node->left) cp_avlnode_multi_print(node->left, level + 1);
}

void cp_avltree_dump(cp_avltree *tree)
{
	if (tree->root) 
	{
		if ((tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
			cp_avlnode_multi_print(tree->root, 0);
		else
			cp_avlnode_print(tree->root, 0);
	}
}

