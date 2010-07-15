#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "nary.h"
#include "vector.h"

static int cp_narytree_txlock(cp_narytree *tree, int type);
static int cp_narytree_txunlock(cp_narytree *tree);

void cp_narynode_deallocate(cp_narynode *node)
{
	if (node)
	{
		if (node->child) free(node->child);
		if (node->value) free(node->value);
		if (node->key) free(node->key);
		free(node);
	}
}

cp_narynode *cp_narynode_alloc(cp_narytree *tree)
{
	cp_narynode *node = calloc(1, sizeof(cp_narynode));
	if (node == NULL) goto ALLOC_ERROR;

	node->key = calloc(tree->order - 1, sizeof(void *));
	if (node->key == NULL) goto ALLOC_ERROR;
	node->value = calloc(tree->order - 1, sizeof(void *));
	if (node->value == NULL) goto ALLOC_ERROR;
	node->child = calloc(tree->order, sizeof(cp_narynode *));
	if (node->child == NULL) goto ALLOC_ERROR;

	return node;
	
ALLOC_ERROR:
	cp_narynode_deallocate(node);
	return NULL;
}

/* narynode_shift
 *
 *  +---+---+---+---+---+           +---+---+---+---+---+
 *  | A | B | C | D | - |    =>     | A | - | B | C | D |
 *  +---+---+---+---+---+           +---+---+---+---+---+
 *  
 */
static void narynode_shift(cp_narynode *node, int index)
{
	int len = node->items - index;
	memmove(&node->key[index + 1], &node->key[index], len * sizeof(void *));
	memmove(&node->value[index + 1], &node->value[index], len * sizeof(void *));
	memmove(&node->child[index + 1], &node->child[index + 0], 
			(len + 1) * sizeof(cp_narynode *));
}

/* narynode_unshift
 *
 *  +---+---+---+---+---+           +---+---+---+---+---+
 *  | A | B | C | D | E |    =>     | A | C | D | E | - |
 *  +---+---+---+---+---+           +---+---+---+---+---+
 *  
 */
static void narynode_unshift(cp_narynode *node, int index)
{
	int len = node->items - index - 1;
	memmove(&node->key[index], &node->key[index + 1], len * sizeof(void *));
	memmove(&node->value[index], &node->value[index + 1], len * sizeof(void *));
	memmove(&node->child[index + 1], &node->child[index + 2], 
			len * sizeof(cp_narynode *));
}

static void narytree_split(cp_narytree *tree, 
		         		cp_narynode *parent,
					 	cp_narynode *node, 
				 		void *key, 
				 		void *value);

/* On Splitting
 * ------------
 *
 *  if O is the order of the tree (number of children per node), i is the 
 *  index where the insertion would occur if the node were not full, find m, 
 *  the key to be moved up to the parent node. 
 *  
 *  let k = (O - 1) / 2 
 *
 * 
 *   O = 2, k = 0             O = 3, k = 1             O = 4, k = 1 
 *      +---+                  +---+---+              +---+---+---+ 
 *      | 0 |                  | 0 | 1 |              | 0 | 1 | 2 | 
 *      +---+                  +---+---+              +---+---+---+ 
 *  
 *  i = 0   m = 0, i         i = 0   m = 0           i = 0   m = 0, 1  
 *  i = 1   m = i, 0         i = 1   m = i           i = 1   m = i, 1 
 *                           i = 2   m = 1           i = 2   m = 1, i 
 *                                                   i = 3   m = 1, 2 
 * 
 * 
 *       O = 5, k = 2               O = 6, k = 2 
 *    +---+---+---+---+         +---+---+---+---+---+ 
 *    | 0 | 1 | 2 | 3 |         | 0 | 1 | 2 | 3 | 4 | 
 *    +---+---+---+---+         +---+---+---+---+---+ 
 * 
 *     i = 0   m = 1              i = 0   m = 1, 2 
 *     i = 1   m = 1              i = 1   m = 1, 2 
 *     i = 2   m = i              i = 2   m = i, 2 
 *     i = 3   m = 2              i = 3   m = 2, i 
 *     i = 4   m = 2              i = 4   m = 2, 3 
 *                                i = 5   m = 2, 3 
 *
 *
 *  It is immediately apparent that for all cases, we may choose
 *
 *  m = k - 1    i < k     (a)
 *      i        i = k     (b)
 *      k        i > k     (c)
 *
 * -------------------------------------------------------------------------
 * inner_split copies about half of the existing node's content into a newly
 * created node. 
 * 
 * narytree_split runs after a node has been split. One mapping moves up to the 
 * parent node, and a link to the newly created node is added.
 */
static void inner_split(cp_narytree *tree, 
				 		cp_narynode *node, 
				 		int index, 
				 		void *key, 
				 		void *value, 
				 		cp_narynode *child)
	
{
	void *swapkey, *swapval;

	int m; /* the index moving up the tree */
	int k = (tree->order - 1) / 2; /* minimum fill factor */
	int j; /* just a loop counter */
	cp_narynode *sibl = cp_narynode_alloc(tree);
	sibl->parent = node->parent;
	if (index < k) /* case (a) */
	{
		m = k - 1;
		swapkey = node->key[m];
		swapval = node->value[m];

		sibl->items = tree->order - k - 1;
		memcpy(&sibl->key[0], &node->key[k], 
			   sibl->items * sizeof(void *));
		memcpy(&sibl->value[0], &node->value[k], 
			   sibl->items * sizeof(void *));
		memcpy(&sibl->child[0], &node->child[k], 
			   (sibl->items + 1) * sizeof(cp_narynode *));
		node->items -= sibl->items;
		if (index < k - 1)
			narynode_shift(node, index);
			
		node->key[index] = key;
		if (!(tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
			node->value[index] = value;
		else
		{
			node->value[index] = cp_vector_create(1);
			cp_vector_add_element(node->value[index], value);
		}
		node->child[index + 1] = child;
	}
	else if (index == k) /* case (b) */
	{
		m = index;
		swapkey = key;
		swapval = value;

		sibl->items = tree->order - k - 1;
		memcpy(&sibl->key[0], &node->key[k], 
			   sibl->items * sizeof(void *));
		memcpy(&sibl->value[0], &node->value[k], 
			   sibl->items * sizeof(void *));
		memcpy(&sibl->child[1], &node->child[k + 1], 
			   sibl->items * sizeof(cp_narynode *));
		sibl->child[0] = child;
		node->items -= sibl->items;
	}
	else /* index > k  --  case (c) */
	{
		int siblindex = index - (k + 1);
		m = k;
		swapkey = node->key[m];
		swapval = node->value[m];

		sibl->items = tree->order - k - 2;

		memcpy(&sibl->key[0], &node->key[k + 1], 
			   sibl->items * sizeof(void *));
		memcpy(&sibl->value[0], &node->value[k + 1],
			   sibl->items * sizeof(void *));
		memcpy(&sibl->child[0], &node->child[k + 1], 
			   (sibl->items + 1) * sizeof(cp_narynode *));

		if (siblindex < sibl->items)
			narynode_shift(sibl, siblindex);

		sibl->key[siblindex] = key;
		if (!(tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
			sibl->value[siblindex] = value;
		else
		{
			sibl->value[siblindex] = cp_vector_create(1);
			cp_vector_add_element(sibl->value[siblindex], value);
		}
		sibl->child[siblindex + 1] = child;
		sibl->items++;
		node->items -= sibl->items;
	}
	for (j = 0; j <= sibl->items; j++) /* reassign parent */
		if (sibl->child[j]) 
			sibl->child[j]->parent = sibl;
	narytree_split(tree, node->parent, sibl, swapkey, swapval);
}

/* narytree_split - complete the split operation by adding the given mapping to
 * the parent node
 */
static void narytree_split(cp_narytree *tree, 
		         		cp_narynode *parent,
				 		cp_narynode *node, 
				 		void *key, 
				 		void *value)
{
	int index = 0;

	/* (1) split at root */
	if (parent == NULL)
	{
		parent = cp_narynode_alloc(tree);
		if (parent == NULL) return;
		parent->child[0] = tree->root;
		tree->root->parent = parent;
		parent->child[1] = node;
		node->parent = parent;
		parent->key[0] = key;
		parent->value[0] = value;
		parent->items = 1;
		tree->root = parent;
		return;
	}

	//~~ replace with binary search
	/* (2) find where new key fits in */
	while (index < parent->items && 
		   (*tree->cmp)(key, parent->key[index]) > 0) index++;

	if (parent->items == tree->order - 1) /* (3) split again */
	{
		inner_split(tree, parent, index, key, value, node);
		return;
	}

	/* (4) insert new node */
	if (index < parent->items)
		narynode_shift(parent, index);
	
	parent->key[index] = key;
	parent->value[index] = value;
	parent->child[index + 1] = node;
	parent->items++;
	node->parent = parent;
}
	
/* locate_mapping - run a binary search to find where a new mapping should be 
 * put 
 */
static cp_narynode *
	locate_mapping(cp_narytree *tree, void *key, int *idx, int *exists)
{
	cp_narynode *curr = tree->root;
	int min, max;
	int pos = 0, cmp = 0;

	while (curr)
	{
		min = -1;
		max = curr->items;
		while (max > min + 1)
		{
			pos = (min + max) / 2;
			if ((cmp = (*tree->cmp)(key, curr->key[pos])) == 0)
			{
				*idx = pos;
				*exists = 1;
				return curr;
			}
			if (cmp > 0)
				min = pos;
			else
				max = pos;
		}
		if (curr->child[max] == NULL)
		{
			*idx = max;
			*exists = 0;
			break;
		}
		curr = curr->child[max];
	}

	return curr;
}

/* the insert operation is different from the split operation in that ``node''
 * is not yet in the tree.
 */
void *cp_narynode_insert(cp_narytree *tree, 
						 void *key, 
						 void *value)
{
	int index = 0;
	int exists = 0;
	cp_narynode *node;
	void *res = NULL;

	/* (1) find where new key fits in */
	node = locate_mapping(tree, key, &index, &exists);

	if (exists) /* (2) replace */
	{
		if (tree->mode & COLLECTION_MODE_DEEP)
		{
			if (tree->key_dtr) (*tree->key_dtr)(node->key[index]);
			if (tree->value_dtr && 
				!(tree->mode & COLLECTION_MODE_MULTIPLE_VALUES)) 
				(*tree->value_dtr)(node->value[index]);
		}

		/* copying handled by caller (in cp_narytree_insert) */
		node->key[index] = key;
		if (!(tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
			res = node->value[index] = value;
		else
			res = cp_vector_add_element(node->value[index], value);

		if (node->items == 0) node->items = 1;
		goto INSERT_DONE;
	}
		
	/* inserting in this node */
	tree->items++;
	if (node->items == tree->order - 1) /* (4) node full, split */
	{
		inner_split(tree, node, index, key, value, NULL);
		goto INSERT_DONE;
	}

	/* (3) add to this node */
	if (index < node->items) /* shift unless inserting at end */
		narynode_shift(node, index);
	
	/* copying handled by caller (in cp_narytree_insert) */
	node->key[index] = key;
	if (!(tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
		node->value[index] = value;
	else
	{
		node->value[index] = cp_vector_create(1);
		cp_vector_add_element(node->value[index], value);
	}
	
	node->items++;

INSERT_DONE:
	return res;
}

void cp_narynode_destroy_deep(cp_narynode *node, cp_narytree *tree)
{
	int i;
	if (node)
	{
		for (i = 0; i < node->items; i++)
		{
			if (node->child[i]) cp_narynode_destroy_deep(node->child[i], tree);
			if (tree->mode & COLLECTION_MODE_DEEP)
			{
				if (tree->key_dtr) (*tree->key_dtr)(node->key[i]);
				if (tree->value_dtr) (*tree->value_dtr)(node->value[i]);
			}
		}
		if (node->child[i]) cp_narynode_destroy_deep(node->child[i], tree);
		free(node->key);
		free(node->value);
		free(node->child);
		free(node);
	}
}


cp_narytree *cp_narytree_create(int order, cp_compare_fn cmp)
{
	return cp_narytree_create_by_option(0, order, cmp, NULL, NULL, NULL, NULL);
}

cp_narytree *cp_narytree_create_by_option(int mode, 
										  int order, 
										  cp_compare_fn cmp, 
										  cp_copy_fn key_copy,
										  cp_destructor_fn key_dtr, 
										  cp_copy_fn value_copy, 
										  cp_destructor_fn value_dtr)
{
	cp_narytree *tree = calloc(1, sizeof(cp_narytree));
	if (tree == NULL) return NULL;

	tree->mode = mode;

	if (!(mode & COLLECTION_MODE_NOSYNC))
	{
		tree->lock = (cp_lock *) malloc(sizeof(cp_lock));
		if (tree->lock == NULL) goto CREATE_ERR;
		if (cp_lock_init(tree->lock, NULL)) goto CREATE_ERR;
	}
		
	tree->order = order;

	tree->cmp = cmp;
	tree->key_copy = key_copy;
	tree->key_dtr = key_dtr;
	tree->value_copy = value_copy;
	tree->value_dtr = value_dtr;
	
	return tree;

CREATE_ERR:
	if (tree->lock) free(tree->lock);
	free(tree);
	return NULL;
}


void cp_narytree_destroy(cp_narytree *tree)
{
	if (tree)
	{
		cp_narynode_destroy_deep(tree->root, tree);
		free(tree);
	}
}

void cp_narytree_destroy_custom(cp_narytree *tree, 
								cp_destructor_fn key_dtr, 
								cp_destructor_fn value_dtr)
{
	tree->key_dtr = key_dtr;
	tree->value_dtr = value_dtr;
	cp_narytree_destroy(tree);
}


void *cp_narytree_insert(cp_narytree *tree, void *key, void *value)
{
	void *res;
	void *dupkey, *dupval;
	
	if (cp_narytree_txlock(tree, COLLECTION_LOCK_WRITE)) return NULL;

	if (tree->mode & COLLECTION_MODE_COPY)
	{
		dupkey = tree->key_copy ? (*tree->key_copy)(key) : key;
		dupval = tree->value_copy ? (*tree->value_copy)(value) : value;
	}
	else
	{
		dupkey = key;
		dupval = value;
	}

	if (tree->root == NULL)
		tree->root = cp_narynode_alloc(tree);

	res = cp_narynode_insert(tree, dupkey, dupval);

	cp_narytree_txunlock(tree);
	return res;
}

static void narynode_unify(cp_narytree *tree, cp_narynode *node);

/* On Unification
 *
 * narynode_unify - borrow left
 *
 *             +---+---+---+---+                      +---+---+---+---+
 *             | A | Q | T |   |                      | A | M | T |   |
 *             +---+---+---+---+                      +---+---+---+---+
 *                /     \               =>               /     \
 * +---+---+---+---+  +---+---+---+---+   +---+---+---+---+  +---+---+---+---+
 * | E | J | M | - |  | R |   |   |   |   | E | J |   |   |  | Q | R |   |   |
 * +---+---+---+---+  +---+---+---+---+   +---+---+---+---+  +---+---+---+---+
 *
 * narynode_unify - borrow right
 *
 *             +---+---+---+---+                      +---+---+---+---+
 *             | A | J | T |   |                      | A | M | T |   |
 *             +---+---+---+---+                      +---+---+---+---+
 *                /     \               =>               /     \
 * +---+---+---+---+  +---+---+---+---+   +---+---+---+---+  +---+---+---+---+
 * | E | - | - | - |  | M | Q | R |   |   | E | J |   |   |  | Q | R |   |   |
 * +---+---+---+---+  +---+---+---+---+   +---+---+---+---+  +---+---+---+---+
 *
 * 
 * left_unify 
 * 
 *               +---+---+---+---+                   +---+---+---+---+
 *               | A | J | T |   |                   | A | T |   |   |
 *               +---+---+---+---+                   +---+---+---+---+
 *                  /     \                   =>        /
 *   +---+---+---+---+   +---+---+---+---+            +---+---+---+---+
 *   | E | - | - | - |   | M | Q |   |   |            | E | J | M | Q |
 *   +---+---+---+---+   +---+---+---+---+            +---+---+---+---+
 *
 *   if the parent node goes below k = (order - 1) / 2 items, narynode_unify
 *   is called on the parent.
 */
static void left_unify(cp_narytree *tree, cp_narynode *node, int idx)
{
	int i;
	cp_narynode *surr = node->parent;
	cp_narynode *sibl = surr->child[idx + 1];
	
	node->key[node->items] = surr->key[idx];
	node->value[node->items] = surr->value[idx];
//~~ possible?
//	if (idx + 2 == tree->order)
//		surr->child[idx + 1] = NULL;
//	else
	surr->child[idx + 1] = surr->child[idx + 2]; 
	narynode_unshift(surr, idx);
	surr->items--;
	node->items++;
	memcpy(&node->key[node->items], &sibl->key[0], 
		   sibl->items * sizeof(void *));
	memcpy(&node->value[node->items], &sibl->value[0], 
		   sibl->items * sizeof(void *));
	for (i = 0; i <= sibl->items; i++) 
		if (sibl->child[i]) sibl->child[i]->parent = node;
	memcpy(&node->child[node->items], &sibl->child[0], 
		   (sibl->items + 1) * sizeof(cp_narynode *));
	node->items += sibl->items;

	cp_narynode_deallocate(sibl);
	narynode_unify(tree, surr);
}

/* node may have less than k items - if so, unify with other nodes */
static void narynode_unify(cp_narytree *tree, cp_narynode *node)
{
	cp_narynode *surr;
	int i, k;
	int idx = 0; /* prevents gcc warning */

	if (node->parent == NULL) /* root node */
	{
		if (node->items	> 0) return;
		if (node->child[0])
		{
			tree->root = node->child[0];
			node->child[0]->parent = NULL;
			cp_narynode_deallocate(node);
		}
		return;
	}

	k = (tree->order - 1) / 2;
	if (node->items >= k) return; /* narytree condition met - no problem */

	/* try to borrow a mapping from siblings */
	surr = node->parent;
	for (i = 0; i <= surr->items; i++)
		if (surr->child[i] == node)
		{
			idx = i;
			break;
		}
	/* idx now holds the current node's index in the parent's child[] array */
	
	if (idx > 0 &&                           /* try borrow from left sibling */
		surr->child[idx - 1] && 
		surr->child[idx - 1]->items > k) 
	{
		cp_narynode *sibl = surr->child[idx - 1];
		narynode_shift(node, 0);
		node->key[0] = surr->key[idx - 1];
		node->value[0] = surr->value[idx - 1];
		surr->key[idx - 1] = sibl->key[sibl->items - 1];
		surr->value[idx - 1] = sibl->value[sibl->items - 1];
		node->items++;
		node->child[0] = sibl->child[sibl->items];
		if (node->child[0]) node->child[0]->parent = node; 
		sibl->items--;
	}
	else if (idx < surr->items &&            /* try borrow from right sibling */
			 surr->child[idx + 1] && 
			 surr->child[idx + 1]->items > k) 
	{
		cp_narynode *sibl = surr->child[idx + 1];
		node->key[node->items] = surr->key[idx];
		node->value[node->items] = surr->value[idx];
		surr->key[idx] = sibl->key[0];
		surr->value[idx] = sibl->value[0];
		node->items++;
		node->child[node->items] = sibl->child[0];
		if (node->child[node->items])
			node->child[node->items]->parent = node;
		sibl->child[0] = sibl->child[1];
		narynode_unshift(sibl, 0);
		sibl->items--;
	}
	else                                     /* siblings too small - unify */
	{
		if (idx > 0 && surr->child[idx - 1])
			left_unify(tree, surr->child[idx - 1], idx - 1);
		else if (idx < surr->items && surr->child[idx + 1])
			left_unify(tree, node, idx);
	}
}

/* find_mapping - performs a binary search on tree nodes to find the mapping 
 * for the given key. On average this saves comparisons for tree orders above 
 * 8 more or less.
 *
 * The node containing the mapping is returned and *idx is set to the index
 * of the requested mapping. 
 */
static cp_narynode *find_mapping(cp_narytree *tree, void *key, int *idx)
{
	cp_narynode *curr = tree->root;
	int min, max;
	int pos = 0, cmp = 0;

	while (curr)
	{
		min = -1;
		max = curr->items;
		while (max > min + 1)
		{
			pos = (min + max) / 2;
			if ((cmp = (*tree->cmp)(key, curr->key[pos])) == 0)
			{
				*idx = pos;
				return curr;
			}
			if (cmp > 0)
				min = pos;
			else
				max = pos;
		}
		curr = curr->child[max];
	}

	return curr;
}

void *cp_narytree_get(cp_narytree *tree, void *key)
{
	void *res = NULL;
	int idx;
	cp_narynode *node;

	if (cp_narytree_txlock(tree, COLLECTION_LOCK_READ)) return NULL;

	node = find_mapping(tree, key, &idx);
	if (node) res = node->value[idx];

	cp_narytree_txunlock(tree);
	
	return res;
}

int cp_narytree_contains(cp_narytree *tree, void *key)
{
	int x;
	int found;

	if (cp_narytree_txlock(tree, COLLECTION_LOCK_READ)) return -1;

	found = (find_mapping(tree, key, &x) != NULL);

	cp_narytree_txunlock(tree);

	return found;
}

/* cp_narytree_delete
 *
 * the following cases are handled:
 *
 * (1) simple case: the node containing the deleted item is a leaf and has 
 *     order/2 items or more after the deletion - just remove the mapping.
 *
 * (2) the node containing the deleted item is not a leaf - replace the item
 *     with one from the sub-node
 */
void *cp_narytree_delete(cp_narytree *tree, void *key)
{
	int i;
	cp_narynode *curr;
	void *res = NULL;
	
	if (cp_narytree_txlock(tree, COLLECTION_LOCK_WRITE)) return NULL;

	/* find mapping to remove */
	curr = find_mapping(tree, key, &i);
	if (curr)
	{
		res = curr->value[i];
		if (curr->child[i + 1]) /* not leaf */
		{
			cp_narynode *leaf = curr->child[i + 1];
			while (leaf->child[0])
				leaf = leaf->child[0];
			if (tree->mode & COLLECTION_MODE_DEEP)
			{
				if (tree->key_dtr) (*tree->key_dtr)(curr->key[i]);
				if (tree->value_dtr) (*tree->value_dtr)(curr->value[i]);
			}
			curr->key[i] = leaf->key[0];
			curr->value[i] = leaf->value[0];
			narynode_unshift(leaf, 0);
			leaf->items--;
			narynode_unify(tree, leaf);
		}
		else /* leaf-like */
		{
			if (tree->mode & COLLECTION_MODE_DEEP)
			{
				if (tree->key_dtr) (*tree->key_dtr)(curr->key[i]);
				if (tree->value_dtr) (*tree->value_dtr)(curr->value[i]);
			}
			narynode_unshift(curr, i);
			curr->items--;
			narynode_unify(tree, curr);
		}
		tree->items--;
	}

	cp_narytree_txunlock(tree);
	return res;
}

static int cp_narytree_lock_internal(cp_narytree *tree, int type)
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

static int cp_narytree_unlock_internal(cp_narytree *tree)
{
	return cp_lock_unlock(tree->lock);
}

static int cp_narytree_txlock(cp_narytree *tree, int type)
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
	return cp_narytree_lock_internal(tree, type);
}

static int cp_narytree_txunlock(cp_narytree *tree)
{
	if (tree->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (tree->mode & COLLECTION_MODE_IN_TRANSACTION && 
		tree->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, tree->txowner)) return 0;
	}
	return cp_narytree_unlock_internal(tree);
}

/* lock and set the transaction indicators */
int cp_narytree_lock(cp_narytree *tree, int type)
{
	int rc;
	if ((tree->mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	if ((rc = cp_narytree_lock_internal(tree, type))) return rc;
	tree->txtype = type;
	tree->txowner = cp_thread_self();
	tree->mode |= COLLECTION_MODE_IN_TRANSACTION;
	return 0;
}

/* unset the transaction indicators and unlock */
int cp_narytree_unlock(cp_narytree *tree)
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
	return cp_narytree_unlock_internal(tree);
}

/* set mode bits on the tree mode indicator */
int cp_narytree_set_mode(cp_narytree *tree, int mode)
{
	int rc;
	int nosync; 

	/* can't set NOSYNC in the middle of a transaction */
	if ((tree->mode & COLLECTION_MODE_IN_TRANSACTION) && 
		(mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	/* COLLECTION_MODE_MULTIPLE_VALUES must be set at creation time */	
	if (mode & COLLECTION_MODE_MULTIPLE_VALUES) return EINVAL;

	if ((rc = cp_narytree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
	nosync = tree->mode & COLLECTION_MODE_NOSYNC;

	tree->mode |= mode;

	if (!nosync)
		cp_narytree_txunlock(tree);

	return 0;
}

/* unset mode bits on the tree mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the tree was not previously synchronized, the 
 * internal synchronization structure is initialized.
 */
int cp_narytree_unset_mode(cp_narytree *tree, int mode)
{
	int rc;
	int nosync;

	/* COLLECTION_MODE_MULTIPLE_VALUES can't be unset */
	if (mode & COLLECTION_MODE_MULTIPLE_VALUES) return EINVAL;

	if ((rc = cp_narytree_txlock(tree, COLLECTION_LOCK_WRITE))) return rc;
	
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
		cp_narytree_txunlock(tree);

	return 0;
}

int cp_narytree_get_mode(cp_narytree *tree)
{
    return tree->mode;
}


static int narytree_scan(cp_narynode *node, 
					  	 cp_callback_fn callback, 
					  	 void *prm)
{
	int i;
	int rc;

	cp_mapping m;
	
	if (node) 
	{
		for (i = 0; i < node->items; i++)
		{
			if ((rc = narytree_scan(node->child[i], callback, prm))) return rc;
			m.key = node->key[i];
			m.value = node->value[i];
			if ((*callback)(&m, prm)) return rc;
		}
		if ((rc = narytree_scan(node->child[i], callback, prm))) return rc;
	}

	return 0;
}

int cp_narytree_callback(cp_narytree *tree, 
						 cp_callback_fn callback, 
						 void *prm)
{
	int rc;
	if ((rc = cp_narytree_txlock(tree, COLLECTION_LOCK_READ))) return rc;
	rc = narytree_scan(tree->root, callback, prm);
	cp_narytree_txunlock(tree);
	return rc;
}

int cp_narytree_count(cp_narytree *tree)
{
	return tree->items;
}


void cp_narytree_print_level(cp_narytree *tree, 
	                      cp_narynode *node, 
					 	  int level, 
						  int curr, 
					 	  int *more)
{
	int i;
	if (node == NULL) return;
	
	if (curr < level)
	{
		for (i = 0; i <= node->items; i++)
			cp_narytree_print_level(tree, node->child[i], level, curr + 1, more);
	}
	else
	{
		printf("(%d) ", node->items);
		printf("[%lX;%lX] ", (long) node, (long) node->parent);
		for (i = 0; i < tree->order - 1; i++)
		{
			if (i <= node->items && node->child[i]) printf("<%lX>", (long) node->child[i]);
			printf("|%s|", (char *) (i < node->items && node->key[i] ? node->key[i] : "-"));
			if (node->child[i]) *more = 1;
		}
		if (node->child[i]) *more = 1;
		if (i == node->items && node->child[i]) printf("<%lX>   ", (long) node->child[i]);
	}
}

void cp_narytree_dump(cp_narytree *tree)
{
	int more;
	int level = 0;
	do
	{
		more = 0;
		cp_narytree_print_level(tree, tree->root, level, 0, &more);
		printf("\n");
		level++;
	} while (more);
}

