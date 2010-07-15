#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "collection.h"
#include "vector.h"
#include "sorted_hash.h"

cp_sh_entry *cp_sh_entry_create(void *key, void *value, cp_mempool *pool)
{
	cp_sh_entry *node;
	
	if (pool) 
		node = (cp_sh_entry *) cp_mempool_calloc(pool);
	else
		node = (cp_sh_entry *) calloc(1, sizeof(cp_sh_entry));

	if (node == NULL) return NULL;

	node->key = key;
	node->value = value;

	return node;
}
	
/* implement COLLECTION_MODE_COPY if set */
static cp_sh_entry *create_sh_entry(cp_sorted_hash *tree, void *key, void *value)
{
	if (tree->mode & COLLECTION_MODE_COPY)
	{
		void *k, *v;
		k = tree->key_copy ? (*tree->key_copy)(key) : key;
		if (k)
		{
			v = tree->value_copy ? (*tree->value_copy)(value) : value;
			if (v)
			{
				if (tree->mode & COLLECTION_MODE_MULTIPLE_VALUES)
				{
					cp_vector *m = cp_vector_create(1);
					if (m == NULL) return NULL;
					cp_vector_add_element(m, v);
					v = m;
				}
				return cp_sh_entry_create(k, v, tree->mempool);
			}
		}
	}
	else if (tree->mode & COLLECTION_MODE_MULTIPLE_VALUES)
	{
		cp_vector *m = cp_vector_create(1);
		if (m == NULL) return NULL;
		cp_vector_add_element(m, value);
		return cp_sh_entry_create(key, m, tree->mempool);
	}
	else 
		return cp_sh_entry_create(key, value, tree->mempool);

	return NULL;
}

void cp_sorted_hash_destroy_entry_deep(cp_sorted_hash *owner, cp_sh_entry *node)
{
    while (node)
    {
        if (node->right)
        {
            node = node->right;
            node->up->right = NULL;
        }
        else if (node->left)
        {
            node = node->left;
            node->up->left = NULL;
        }
        else
        {
            cp_sh_entry *tmp = node;
			if (node->multiple)
			{
				cp_sh_entry *m = node->multiple;
				cp_sh_entry *n;
				do
				{
					n = m->multiple;
					cp_sorted_hash_destroy_entry(owner, m);
					m = n;
				} while (m);
			}
            node = node->up;
            cp_sorted_hash_destroy_entry(owner, tmp);
        }
    }
}

void cp_sorted_hash_destroy_entry(cp_sorted_hash *tree, cp_sh_entry *node)
{
	if (node)
	{
		if ((tree->mode & COLLECTION_MODE_DEEP))
		{
			if (tree->key_dtr) (*tree->key_dtr)(node->key);
			if (tree->mode & COLLECTION_MODE_MULTIPLE_VALUES)
			{
				if (node->value)
					cp_vector_destroy_custom(node->value, tree->value_dtr);
			}
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


cp_sorted_hash *cp_sorted_hash_create(cp_hashfunction hash, 
									  cp_compare_fn cmp_key, 
									  cp_mapping_cmp_fn cmp_mapping)
{
	cp_sorted_hash *tree = calloc(1, sizeof(cp_sorted_hash));
	if (tree == NULL) return NULL;

	tree->size = 11;
	tree->table = (cp_sh_entry **) calloc(tree->size, sizeof(cp_sh_entry));
	if (tree->table == NULL)
	{
		free(tree);
		return NULL;
	}

	tree->mode = COLLECTION_MODE_NOSYNC;
	tree->hash = hash;
	tree->cmp_key = cmp_key;
	tree->cmp_mapping = cmp_mapping;

	return tree;
}

/*
 * complete parameter create function
 */
cp_sorted_hash *
	cp_sorted_hash_create_by_option(int mode, 
									unsigned long size_hint,
									cp_hashfunction hash, 
									cp_compare_fn cmp_key, 
									cp_mapping_cmp_fn cmp_mapping, 
									cp_copy_fn key_copy, 
									cp_destructor_fn key_dtr,
							   		cp_copy_fn val_copy, 
									cp_destructor_fn val_dtr)
{
	cp_sorted_hash *tree = calloc(1, sizeof(cp_sorted_hash));
	if (tree == NULL) return NULL;

	tree->size = cp_hashtable_choose_size(size_hint);
	tree->table = (cp_sh_entry **) calloc(tree->size, sizeof(cp_sh_entry));
	if (tree->table == NULL)
	{
		free(tree);
		return NULL;
	}

	tree->mode = COLLECTION_MODE_NOSYNC;
	tree->hash = hash;
	tree->cmp_key = cmp_key;
	tree->cmp_mapping = cmp_mapping;
	
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
			cp_sorted_hash_destroy(tree);
			return NULL;
		}
		if (cp_lock_init(tree->lock, NULL) != 0)
		{
			cp_sorted_hash_destroy(tree);
			return NULL;
		}
	}

	return tree;
}


void cp_sorted_hash_destroy(cp_sorted_hash *tree)
{
	if (tree)
	{
		cp_sorted_hash_destroy_entry_deep(tree, tree->root);
		if (tree->lock)
		{
			cp_lock_destroy(tree->lock);
			free(tree->lock);
		}
		free(tree->table);
		free(tree);
	}
}

void cp_sorted_hash_destroy_custom(cp_sorted_hash *tree, 
							  cp_destructor_fn key_dtr,
							  cp_destructor_fn val_dtr)
{
	tree->mode |= COLLECTION_MODE_DEEP;
	tree->key_dtr = key_dtr;
	tree->value_dtr = val_dtr;
	cp_sorted_hash_destroy(tree);
}

static int cp_sorted_hash_lock_internal(cp_sorted_hash *tree, int type)
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

static int cp_sorted_hash_unlock_internal(cp_sorted_hash *tree)
{
	return cp_lock_unlock(tree->lock);
}

int cp_sorted_hash_txlock(cp_sorted_hash *tree, int type)
{
	/* clear errno to allow client code to distinguish between a NULL return
	 * value indicating the tree doesn't contain the requested value and NULL
	 * on locking failure in tree operations
	 */
	if (tree->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (tree->mode & COLLECTION_MODE_IN_TRANSACTION && 
		tree->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, tree->txowner)) return 0;
	}
	errno = 0;
	return cp_sorted_hash_lock_internal(tree, type);
}

int cp_sorted_hash_txunlock(cp_sorted_hash *tree)
{
	if (tree->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (tree->mode & COLLECTION_MODE_IN_TRANSACTION && 
		tree->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, tree->txowner)) return 0;
	}
	return cp_sorted_hash_unlock_internal(tree);
}

/* lock and set the transaction indicators */
int cp_sorted_hash_lock(cp_sorted_hash *tree, int type)
{
	int rc;
	if ((tree->mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	if ((rc = cp_sorted_hash_lock_internal(tree, type))) return rc;
	tree->txtype = type;
	tree->txowner = cp_thread_self();
	tree->mode |= COLLECTION_MODE_IN_TRANSACTION;
	return 0;
}

/* unset the transaction indicators and unlock */
int cp_sorted_hash_unlock(cp_sorted_hash *tree)
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
	return cp_sorted_hash_unlock_internal(tree);
}

/* set mode bits on the tree mode indicator */
int cp_sorted_hash_set_mode(cp_sorted_hash *tree, int mode)
{
	int rc;
	int nosync; 

	/* can't set NOSYNC in the middle of a transaction */
	if ((tree->mode & COLLECTION_MODE_IN_TRANSACTION) && 
		(mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	/* COLLECTION_MODE_MULTIPLE_VALUES must be set at creation time */	
	if (mode & COLLECTION_MODE_MULTIPLE_VALUES) return EINVAL;

	if ((rc = cp_sorted_hash_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
	nosync = tree->mode & COLLECTION_MODE_NOSYNC;

	tree->mode |= mode;

	if (!nosync)
		cp_sorted_hash_txunlock(tree);

	return 0;
}

/* unset mode bits on the tree mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the tree was not previously synchronized, the 
 * internal synchronization structure is initialized.
 */
int cp_sorted_hash_unset_mode(cp_sorted_hash *tree, int mode)
{
	int rc;
	int nosync;

	/* COLLECTION_MODE_MULTIPLE_VALUES can't be unset */
	if (mode & COLLECTION_MODE_MULTIPLE_VALUES) return EINVAL;

	if ((rc = cp_sorted_hash_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
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
		cp_sorted_hash_txunlock(tree);

	return 0;
}

int cp_sorted_hash_get_mode(cp_sorted_hash *tree)
{
    return tree->mode;
}


static cp_sh_entry *sibling(cp_sh_entry *node)
{
	return node == node->up->left ? node->up->right : node->up->left;
}

static int is_right_child(cp_sh_entry *node)
{
	return (node->up->right == node);
}

static int is_left_child(cp_sh_entry *node)
{
	return (node->up->left == node);
}

/*         left rotate
 *
 *    (P)                (Q)
 *   /   \              /   \
 *  1    (Q)    ==>   (P)    3
 *      /   \        /   \
 *     2     3      1     2
 *
 */
static void left_rotate(cp_sorted_hash *tree, cp_sh_entry *p)
{
	cp_sh_entry *q = p->right;
	cp_sh_entry **sup;
	
	if (p->up)
		sup = is_left_child(p) ? &(p->up->left) : &(p->up->right);
	else
		sup = &tree->root;

	p->right = q->left;
	if (p->right) p->right->up = p;
	q->left = p;
	q->up = p->up;
	p->up = q;
	*sup = q;
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
static void right_rotate(cp_sorted_hash *tree, cp_sh_entry *p)
{
	cp_sh_entry *q = p->left;
	cp_sh_entry **sup;
	
	if (p->up)
		sup = is_left_child(p) ? &(p->up->left) : &(p->up->right);
	else
		sup = &tree->root;

	p->left = q->right;
	if (p->left) p->left->up = p;
	q->right = p;
	q->up = p->up;
	p->up = q;
	*sup = q;
}


/*
 * newly entered node is RED; check balance recursively as required 
 */
static void rebalance(cp_sorted_hash *tree, cp_sh_entry *node)
{
	cp_sh_entry *up = node->up;
	if (up == NULL || up->color == RB_BLACK) return;
	if (sibling(up) && sibling(up)->color == RB_RED)
	{
		up->color = RB_BLACK;
		sibling(up)->color = RB_BLACK;
		if (up->up->up)
		{
			up->up->color = RB_RED;
			rebalance(tree, up->up);
		}
	}
	else
	{
		if (is_left_child(node) && is_right_child(up))
		{
			right_rotate(tree, up);
			node = node->right;
		}
		else if (is_right_child(node) && is_left_child(up))
		{
			left_rotate(tree, up);
			node = node->left;
		}

		node->up->color = RB_BLACK;
		node->up->up->color = RB_RED;

		if (is_left_child(node)) // && is_left_child(node->up)
			right_rotate(tree, node->up->up);
		else 
			left_rotate(tree, node->up->up);
	}
}

/* update_sh_entry - implement COLLECTION_MODE_COPY, COLLECTION_MODE_DEEP and
 * COLLECTION_MODE_MULTIPLE_VALUES when inserting a value for an existing key
 */
static void *
	update_sh_entry(cp_sorted_hash *tree, cp_sh_entry *node, void *key, void *value)
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
	if (tree->mode & COLLECTION_MODE_MULTIPLE_VALUES)
	{
		cp_vector_add_element(node->value, new_value);
		return node->value;
	}
	else
		node->value = new_value;

	return new_value;
}

/*
 * cp_sorted_hash_insert iterates through the tree, finds where the new node 
 * fits in, puts it there, then calls rebalance. 
 *
 * If a mapping for the given key already exists it is replaced unless 
 * COLLECTION_MODE_MULTIPLE_VALUES is set, is which case a new mapping is 
 * added. By default COLLECTION_MODE_MULTIPLE_VALUES is not set.
 */
void *cp_sorted_hash_insert(cp_sorted_hash *tree, void *key, void *value)
{
	void *res = NULL;
	cp_sh_entry *entry, **curr;
	void *ikey, *ivalue;
	unsigned long index;
	
	if (cp_sorted_hash_txlock(tree, COLLECTION_LOCK_WRITE)) return NULL;

	if (tree->mode & COLLECTION_MODE_COPY)
	{
		ikey = tree->key_copy ? (*tree->key_copy)(key) : key;
		ivalue = tree->value_copy ? (*tree->value_copy)(value) : value;
	}
	else
	{
		ikey = key;
		ivalue = value;
	}

	entry = cp_sh_entry_create(ikey, ivalue, tree->mempool);
	if (entry == NULL) goto INSERT_DONE;

	entry->code = (*tree->hash)(key);
	index = entry->code % tree->size;

	curr = &tree->table[index];
	while (*curr) 
	{
		if (!(tree->mode & COLLECTION_MODE_MULTIPLE_VALUES) &&
			(*tree->cmp_key)(key, (*curr)->key) == 0)
		{
			if (tree->mode & COLLECTION_MODE_DEEP)
			{
				if (tree->key_dtr) (*tree->key_dtr)((*curr)->key);
				if (tree->value_dtr) (*tree->value_dtr)((*curr)->value);
			}
			(*curr)->key = ikey;
			(*curr)->value = ivalue;
			cp_sorted_hash_destroy_entry(tree, entry);
			break;
		}
		curr = &(*curr)->bucket;
	}

	if (*curr == NULL) *curr = entry;

	if (tree->root == NULL)
	{
		tree->root = entry;
		res = value;
		tree->root->color = RB_BLACK;
		tree->items++;
	}
	else
	{
		int cmp;
		cp_sh_entry *prev = NULL;
		curr = &tree->root;

		while (*curr)
		{
			prev = *curr;
			if (tree->cmp_mapping)
			{
				cmp = (*tree->cmp_mapping)((cp_mapping *) *curr, 
										   (cp_mapping *) entry);
			}
			else
			{
				cmp = (*tree->cmp_key)((*curr)->key, entry->key);
			}

			if (cmp < 0)
				curr = &(*curr)->right;
			else if (cmp > 0) 
				curr = &(*curr)->left;
			else /* replace */
			{
				prev = *curr;
				curr = &(*curr)->multiple;
				while (*curr)
				{
					(*curr)->multiple_prev = prev;
					prev = *curr;
					curr = &(*curr)->multiple;
				}
				*curr = entry;
				entry->multiple_prev = NULL;
				res = entry->value;
				tree->items++;
				break;
			}
		}

		if (*curr == NULL) /* not replacing, create new node */
		{
			*curr = entry;
			if (*curr == NULL) goto INSERT_DONE;
			res = (*curr)->value;
			tree->items++;
			(*curr)->up = prev;
			rebalance(tree, *curr);
		}
	}

INSERT_DONE:
	cp_sorted_hash_txunlock(tree);
	return res;
}

/* cp_sorted_hash_get - return the value mapped to the given key or NULL if none is
 * found. If COLLECTION_MODE_MULTIPLE_VALUES is set the returned value is a
 * cp_vector object or NULL if no mapping is found. 
 */
void *cp_sorted_hash_get(cp_sorted_hash *tree, void *key)
{
	int c;
	unsigned long index;
	cp_sh_entry *curr;
	void *value = NULL;
	
	if (cp_sorted_hash_txlock(tree, COLLECTION_LOCK_READ)) return NULL;

	index = (*tree->hash)(key) % tree->size;
	curr = tree->table[index];
	//~~ handle mutliple values
	while (curr)
	{
		c = tree->cmp_key(curr->key, key);
		if (c == 0) 
		{
			value = curr->value;
			break;
		}
		curr = curr->bucket;
	}

	cp_sorted_hash_txunlock(tree);
	return value;;
}
	

/* find the value of the closest key by operator. All of this becuase the tree
 * isn't threaded.
 */
CPROPS_DLL
void *cp_sorted_hash_find(cp_sorted_hash *tree, cp_mapping *mapping, cp_op op)
{
	int cmp;
	unsigned long index;
	cp_sh_entry **curr;
	cp_sh_entry *prev;
	cp_sh_entry *res = NULL;

	if (tree->root == NULL) return NULL;

	if (cp_sorted_hash_txlock(tree, COLLECTION_LOCK_READ)) return NULL;

	curr = &tree->root;
	while (*curr)
	{
		prev = *curr;
		if (tree->cmp_mapping)
		{
			cmp = (*tree->cmp_mapping)((cp_mapping *) *curr, mapping);
		}
		else
		{
			cmp = (*tree->cmp_key)((*curr)->key, mapping->key);
		}

		if (cmp == 0) break;
		if (op == CP_OP_NE)
		{
			res = *curr;
			goto FIND_DONE;
		}

		if (cmp < 0)
			curr = &(*curr)->right;
		else 
			curr = &(*curr)->left;
	}

	if (*curr)
	{
		if (op == CP_OP_EQ || op == CP_OP_LE || op == CP_OP_GE)
		{
			res = *curr;
			goto FIND_DONE;
		}

		if (op == CP_OP_GT)
		{
			if ((*curr)->right)
			{
				curr = &(*curr)->right;
				while ((*curr)->left) curr = &(*curr)->left;
				res = *curr;
			}
			else
			{
				while ((*curr)->up)
				{
					prev = *curr;
					curr = &(*curr)->up;
					if ((*curr)->left == prev)
					{
						res = *curr;
						break;
					}
				}
			}
		}
		else
		{
			if ((*curr)->left)
			{
				curr = &(*curr)->left;
				while ((*curr)->right) curr = &(*curr)->right;
				res = *curr;
			}
			else
			{
				while ((*curr)->up)
				{
					prev = *curr;
					curr = &(*curr)->up;
					if ((*curr)->right == prev)
					{
						res = *curr;
						break;
					}
				}
			}
		}

		goto FIND_DONE;
	}

	/* *curr is NULL */
	if (op == CP_OP_EQ) goto FIND_DONE;

	if (op == CP_OP_LT || op == CP_OP_LE)
	{
		if (curr == &prev->right) 
		{
			res = prev;
			goto FIND_DONE;
		}

		while (prev)
		{
			if (prev->up && prev->up->right == prev)
			{
				res = prev->up;
				break;
			}
			prev = prev->up;
		}
	} 
	else /* op == CP_OP_GE || op == CP_OP_GT */
	{
		if (curr == &prev->left)
		{
			res = prev;
			goto FIND_DONE;
		}

		while (prev)
		{
			if (prev->up && prev->up->left == prev)
			{
				res = prev->up;
				break;
			}
			prev = prev->up;
		}
	}

FIND_DONE:
	cp_sorted_hash_txunlock(tree);
	return res->value;
}

int cp_sorted_hash_contains(cp_sorted_hash *tree, void *key)
{
	return (cp_sorted_hash_get(tree, key) != NULL);
}

/* helper function for deletion */
static void swap_node_content(cp_sh_entry *a, cp_sh_entry *b)
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
 * helper function for cp_sorted_hash_delete to remove nodes with either a left 
 * NULL branch or a right NULL branch
 */
static void rb_unlink(cp_sorted_hash *tree, cp_sh_entry *node)
{
	if (node->multiple) node->multiple->multiple_prev = node->multiple_prev;
	if (node->multiple_prev) node->multiple_prev->multiple = node->multiple;

	if (node->left)
	{
		node->left->up = node->up;
		if (node->up)
		{
			if (is_left_child(node))
				node->up->left = node->left;
			else
				node->up->right = node->left;
		}
		else
			tree->root = node->left;
	}
	else
	{
		if (node->right) node->right->up = node->up;
		if (node->up)
		{
			if (is_left_child(node))
				node->up->left = node->right;
			else
				node->up->right = node->right;
		}
		else
			tree->root = node->right;
	}
}

/* delete_rebalance - perform rebalancing after a deletion */
static void delete_rebalance(cp_sorted_hash *tree, cp_sh_entry *n)
{
	if (n->up)
	{
		cp_sh_entry *sibl = sibling(n);

		if (sibl->color == RB_RED)
		{
			n->up->color = RB_RED;
			sibl->color = RB_BLACK;
			if (is_left_child(n))
				left_rotate(tree, n->up);
			else
				right_rotate(tree, n->up);
			sibl = sibling(n);
		}

		if (n->up->color == RB_BLACK &&
			sibl->color == RB_BLACK &&
			(sibl->left == NULL || sibl->left->color == RB_BLACK) &&
			(sibl->right == NULL || sibl->right->color == RB_BLACK))
		{
			sibl->color = RB_RED;
			delete_rebalance(tree, n->up);
		}
		else
		{
			if (n->up->color == RB_RED &&
				sibl->color == RB_BLACK &&
				(sibl->left == NULL || sibl->left->color == RB_BLACK) &&
				(sibl->right == NULL || sibl->right->color == RB_BLACK))
			{
				sibl->color = RB_RED;
				n->up->color = RB_BLACK;
			}
			else
			{
				if (is_left_child(n) && 
					sibl->color == RB_BLACK &&
					sibl->left && sibl->left->color == RB_RED && 
					(sibl->right == NULL || sibl->right->color == RB_BLACK))
				{
					sibl->color = RB_RED;
					sibl->left->color = RB_BLACK;
					right_rotate(tree, sibl);
					
					sibl = sibling(n);
				}
				else if (is_right_child(n) &&
					sibl->color == RB_BLACK &&
					sibl->right && sibl->right->color == RB_RED &&
					(sibl->left == NULL || sibl->left->color == RB_BLACK))
				{
					sibl->color = RB_RED;
					sibl->right->color = RB_BLACK;
					left_rotate(tree, sibl);

					sibl = sibling(n);
				}

				sibl->color = n->up->color;
				n->up->color = RB_BLACK;
				if (is_left_child(n))
				{
					sibl->right->color = RB_BLACK;
					left_rotate(tree, n->up);
				}
				else
				{
					sibl->left->color = RB_BLACK;
					right_rotate(tree, n->up);
				}
			}
		}
	}
}

/* cp_sorted_hash_delete_impl - delete one node from a red black tree */
void *cp_sorted_hash_delete_impl(cp_sorted_hash *tree, void *key)
{
	void *res = NULL;
	cp_sh_entry **table_node;
	unsigned long index;
	cp_sh_entry *node; 
	int cmp;

	//~~ handle multiples
	index = (*tree->hash)(key) % tree->size;
	table_node = &tree->table[index];
	while (*table_node && (*tree->cmp_key)((*table_node)->key, key))
		table_node = &(*table_node)->bucket;

	node = *table_node;

	if (node) /* may be null if not found */
	{
		cp_sh_entry *child; 
		res = node->value;
		tree->items--;

		if (node->right && node->left)
		{
			cp_sh_entry *surrogate = node;
			node = node->right;
			while (node->left) node = node->left;
			swap_node_content(node, surrogate);
		}
		child = node->right ? node->right : node->left;

		/* if the node was red - no rebalancing required */
		if (node->color == RB_BLACK)
		{
			if (child)
			{
				/* single red child - paint it black */
				if (child->color == RB_RED)
					child->color = RB_BLACK; /* and the balance is restored */
				else
					delete_rebalance(tree, child);
			}
			else 
				delete_rebalance(tree, node);
		}

		*table_node = (*table_node)->bucket;
		rb_unlink(tree, node);
		cp_sorted_hash_destroy_entry(tree, node);
		
	}

	return res;
}

/* cp_sorted_hash_delete - deletes the value mapped to the given key from the 
 * tree and returns the value removed. 
 */
void *cp_sorted_hash_delete(cp_sorted_hash *tree, void *key)
{
	void *res = NULL;

	if (cp_sorted_hash_txlock(tree, COLLECTION_LOCK_WRITE)) return NULL;

	res = cp_sorted_hash_delete_impl(tree, key);

	cp_sorted_hash_txunlock(tree);
	return res;
}

static int 
	rb_scan_pre_order(cp_sh_entry *node, cp_callback_fn callback, void *prm)
{
	int rc;
	
	if (node) 
	{
		if ((rc = (*callback)(node, prm))) return rc;
		if (node->multiple)
		{
			cp_sh_entry *multiple = node->multiple;
			do
			{
				if ((rc = (*callback)(multiple, prm))) return rc;
				multiple = multiple->multiple;
			} while (multiple);
		}
		if ((rc = rb_scan_pre_order(node->left, callback, prm))) return rc;
		if ((rc = rb_scan_pre_order(node->right, callback, prm))) return rc;
	}

	return 0;
}

int cp_sorted_hash_callback_preorder(cp_sorted_hash *tree, 
								cp_callback_fn callback, 
								void *prm)
{
	int rc;

	if ((rc = cp_sorted_hash_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = rb_scan_pre_order(tree->root, callback, prm);
	cp_sorted_hash_txunlock(tree);

	return rc;
}

static int 
	rb_scan_in_order(cp_sh_entry *node, cp_callback_fn callback, void *prm)
{
	int rc;
	
	if (node) 
	{
		if ((rc = rb_scan_in_order(node->left, callback, prm))) return rc;
		if ((rc = (*callback)(node, prm))) return rc;
		if (node->multiple)
		{
			cp_sh_entry *multiple = node->multiple;
			do
			{
				if ((rc = (*callback)(multiple, prm))) return rc;
				multiple = multiple->multiple;
			} while (multiple);
		}
		if ((rc = rb_scan_in_order(node->right, callback, prm))) return rc;
	}

	return 0;
}

int cp_sorted_hash_callback(cp_sorted_hash *tree, 
							cp_callback_fn callback, 
							void *prm)
{
	int rc;

	if ((rc = cp_sorted_hash_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = rb_scan_in_order(tree->root, callback, prm);
	cp_sorted_hash_txunlock(tree);

	return rc;
}

static int 
	rb_scan_post_order(cp_sh_entry *node, cp_callback_fn callback, void *prm)
{
	int rc;
	
	if (node) 
	{
		if ((rc = rb_scan_post_order(node->left, callback, prm))) return rc;
		if ((rc = rb_scan_post_order(node->right, callback, prm))) return rc;
		if ((rc = (*callback)(node, prm))) return rc;
		if (node->multiple)
		{
			cp_sh_entry *multiple = node->multiple;
			do
			{
				if ((rc = (*callback)(multiple, prm))) return rc;
				multiple = multiple->multiple;
			} while (multiple);
		}
	}

	return 0;
}

int cp_sorted_hash_callback_postorder(cp_sorted_hash *tree, 
								 cp_callback_fn callback, 
								 void *prm)
{
	int rc;

	if ((rc = cp_sorted_hash_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = rb_scan_post_order(tree->root, callback, prm);
	cp_sorted_hash_txunlock(tree);

	return rc;
}

int cp_sorted_hash_count(cp_sorted_hash *tree)
{
	return tree->items;
}


void cp_sh_entry_print(cp_sh_entry *node, int level)
{
	int i;
	if (node->right) cp_sh_entry_print(node->right, level + 1);
	for (i = 0; i < level; i++) printf("  . ");
	printf("(%d) [%s => %s]\n", node->color, (char *) node->key, (char *) node->value);
	if (node->left) cp_sh_entry_print(node->left, level + 1);
}

void cp_sh_entry_multi_print(cp_sh_entry *node, int level)
{
	int i;
	cp_vector *v = node->value;
	if (node->right) cp_sh_entry_multi_print(node->right, level + 1);
	
	for (i = 0; i < level; i++) printf("  . ");
	printf("(%d) [%s => ", node->color, (char *) node->key);

	for (i = 0; i < cp_vector_size(v); i++)
		printf("%s; ", (char *) cp_vector_element_at(v, i));

	printf("]\n");

	if (node->left) cp_sh_entry_multi_print(node->left, level + 1);
}

void cp_sorted_hash_dump(cp_sorted_hash *tree)
{
	if (tree->root) 
	{
		if (tree->mode & COLLECTION_MODE_MULTIPLE_VALUES)
			cp_sh_entry_multi_print(tree->root, 0);
		else
			cp_sh_entry_print(tree->root, 0);
	}
}

/* set tree to use given mempool or allocate a new one if pool is NULL */
int cp_sorted_hash_use_mempool(cp_sorted_hash *tree, cp_mempool *pool)
{
	int rc = 0;
	
	if ((rc = cp_sorted_hash_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
	if (pool)
	{
		if (pool->item_size < sizeof(cp_sh_entry))
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
										sizeof(cp_sh_entry), 0);
		if (tree->mempool == NULL) 
		{
			rc = ENOMEM;
			goto DONE;
		}
	}

DONE:
	cp_sorted_hash_txunlock(tree);
	return rc;
}


/* set tree to use a shared memory pool */
int cp_sorted_hash_share_mempool(cp_sorted_hash *tree, cp_shared_mempool *pool)
{
	int rc;

	if ((rc = cp_sorted_hash_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;

	if (tree->mempool)
	{
		if (tree->items)
		{
			rc = ENOTEMPTY;
			goto DONE;
		}

		cp_mempool_destroy(tree->mempool);
	}

	tree->mempool = cp_shared_mempool_register(pool, sizeof(cp_sh_entry));
	if (tree->mempool == NULL) 
	{
		rc = ENOMEM;
		goto DONE;
	}
	
DONE:
	cp_sorted_hash_txunlock(tree);
	return rc;
}

