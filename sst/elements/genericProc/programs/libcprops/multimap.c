#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "collection.h"
#include "vector.h"
#include "multimap.h"

cp_index_map_node *
    cp_index_map_node_create(void *entry, cp_mempool *pool)
{
    cp_index_map_node *node;
    
    if (pool) 
        node = (cp_index_map_node *) cp_mempool_calloc(pool);
    else
        node = (cp_index_map_node *) calloc(1, sizeof(cp_index_map_node));

    if (node == NULL) return NULL;

    node->entry = entry;

    return node;
}
    
/* implement COLLECTION_MODE_MULTIPLE_VALUES if set */
static cp_index_map_node *
    create_index_map_node(cp_index_map *tree, void *entry)
{
	if ((tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
    {
        cp_vector *m = cp_vector_create(1);
        if (m == NULL) return NULL;
        cp_vector_add_element(m, entry);
        return cp_index_map_node_create(m, tree->owner->mempool);
    }
    else 
        return cp_index_map_node_create(entry, tree->owner->mempool);

    return NULL;
}

void cp_index_map_destroy_node(cp_index_map *tree, cp_index_map_node *node)
{
    if (node)
    {
		if (node->entry)
		{
        	if ((tree->mode & COLLECTION_MODE_DEEP))
	   	    {
    	   	    if ((tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
        	   	    cp_vector_destroy_custom(node->entry, tree->owner->dtr);
        	    else if (tree->owner->dtr) 
   	        	    (*tree->owner->dtr)(node->entry);
			}
	   	    else if ((tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
    	   	    cp_vector_destroy(node->entry);
        }

        if (tree->owner->mempool)
            cp_mempool_free(tree->owner->mempool, node);
        else
            free(node);
    }
}

void cp_index_map_drop_node(cp_index_map *tree, cp_index_map_node *node)
{
    if (node)
    {
		if (tree->mode & COLLECTION_MODE_MULTIPLE_VALUES && node->entry)
			cp_vector_destroy(node->entry);

        if (tree->owner->mempool)
            cp_mempool_free(tree->owner->mempool, node);
        else
            free(node);
    }
}

void cp_index_map_drop_node_deep(cp_index_map *tree, cp_index_map_node *node)
{
	if (node)
	{
		if (node->left) 
			cp_index_map_drop_node_deep(tree, node->left);
		if (node->right)
			cp_index_map_drop_node_deep(tree, node->right);
		cp_index_map_drop_node(tree, node);
	}
}

void cp_index_map_destroy_ref(cp_index_map *tree)
{
	cp_index_map_drop_node_deep(tree, tree->root);
	free(tree);
}

void cp_index_map_destroy_node_deep(cp_index_map *owner, 
                                    cp_index_map_node *node)
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
            cp_index_map_node *tmp = node;
            node = node->up;
            cp_index_map_destroy_node(owner, tmp);
        }
    }
}

void cp_index_map_destroy(cp_index_map *tree)
{
    if (tree)
    {
        cp_index_map_destroy_node_deep(tree, tree->root);
        free(tree);
    }
}

void *cp_index_map_insert(cp_index_map *tree, void *entry);

cp_multimap *
    cp_multimap_create_by_option(int mode, cp_key_fn key, 
                                 cp_compare_fn cmp, cp_copy_fn copy, 
                                 cp_destructor_fn dtr)
{
	int default_map_mode;
    cp_multimap *map = (cp_multimap *) calloc(1, sizeof(cp_multimap));
    if (map == NULL) goto CREATE_ERROR;

    if (!(mode & COLLECTION_MODE_NOSYNC))
    {
        map->lock = (cp_lock *) malloc(sizeof(cp_lock));
        if (map->lock == NULL) goto CREATE_ERROR;

        if (cp_lock_init(map->lock, NULL) != 0) goto CREATE_ERROR;
    }

    map->mode = mode;

    map->copy = copy;
    map->dtr = dtr;

    map->default_index.type = 
        (mode & COLLECTION_MODE_MULTIPLE_VALUES) ? CP_MULTIPLE : CP_UNIQUE;
    map->default_index.key = key;
    map->default_index.cmp = cmp;

	default_map_mode = 
		(mode & COLLECTION_MODE_MULTIPLE_VALUES) ? (mode & ~(COLLECTION_MODE_COPY | COLLECTION_MODE_DEEP)) : mode;

    map->default_map = 
        cp_index_map_create(map, default_map_mode , &map->default_index);
    if (map->default_map == NULL) goto CREATE_ERROR;

    map->index = 
        cp_hashlist_create_by_option(COLLECTION_MODE_NOSYNC | 
                                     COLLECTION_MODE_DEEP, 
                                     3, 
                                     cp_hash_addr, cp_hash_compare_addr, 
                                     (cp_copy_fn) cp_index_copy, free, 
                                     NULL, 
                                     (cp_destructor_fn) 
									 	cp_index_map_destroy_ref);
    if (map->index == NULL) goto CREATE_ERROR;

	if (mode & COLLECTION_MODE_MULTIPLE_VALUES)
	{
		cp_index *primary_index = (cp_index *) calloc(1, sizeof(cp_index));
		if (primary_index == NULL) goto CREATE_ERROR;
		primary_index->type = CP_UNIQUE;
		primary_index->key = NULL;
		primary_index->cmp = cp_hash_compare_addr;
		map->primary = 
			cp_index_map_create(map, 
								mode & ~(COLLECTION_MODE_MULTIPLE_VALUES), 
								primary_index);
    	if (map->primary == NULL) 
		{
			free(primary_index);
			goto CREATE_ERROR;
		}

		cp_hashlist_append(map->index, &map->default_index, map->default_map);
	}
	else
		map->primary = map->default_map;

    return map;

CREATE_ERROR:
    if (map)
    {
		if ((map->mode & COLLECTION_MODE_MULTIPLE_VALUES) &&
		    map->primary != NULL)
			cp_index_map_destroy(map->primary);
        if (map->default_map) cp_index_map_destroy(map->default_map);
        if (map->index) cp_hashlist_destroy(map->index);
        if (map->lock) cp_lock_destroy(map->lock);
        free(map);
    }
    return NULL;
}

cp_multimap *cp_multimap_create(cp_key_fn key_fn, cp_compare_fn cmp)
{
    return cp_multimap_create_by_option(0, key_fn, cmp, NULL, NULL);
}

void cp_multimap_destroy(cp_multimap *map)
{
	if (map)
	{
		cp_hashlist_remove_by_option(map->index, &map->default_index, COLLECTION_MODE_NOSYNC);
		if ((map->mode & COLLECTION_MODE_MULTIPLE_VALUES))
		{
			free(map->primary->index);
//			cp_hashlist_remove_by_option(map->index, &map->default_index, COLLECTION_MODE_NOSYNC);
			cp_hashlist_destroy(map->index);
			if (map->default_map) cp_index_map_destroy_ref(map->default_map);
		}
		else
		{
			if (map->index) cp_hashlist_destroy(map->index);
		}
		if (map->primary) cp_index_map_destroy(map->primary);
		if (map->lock) cp_lock_destroy(map->lock);
		free(map);
	}
}

void cp_multimap_destroy_custom(cp_multimap *map, cp_destructor_fn dtr)
{
	if (map)
	{
		map->dtr = dtr;
		cp_multimap_destroy(map);
	}
}

cp_index_map *
    cp_index_map_create(cp_multimap *owner, int mode, cp_index *index)
{
    cp_index_map *tree;

    if (index == NULL) return NULL;

    tree = calloc(1, sizeof(cp_index_map));
    if (tree == NULL) return NULL;

    tree->index = index;
    tree->mode = mode;

    tree->owner = owner;

    return tree;
}

void cp_index_map_destroy_custom(cp_index_map *tree, cp_destructor_fn dtr)
{
    tree->mode |= COLLECTION_MODE_DEEP;
    tree->owner->dtr = dtr;
    cp_index_map_destroy(tree);
}

static int cp_multimap_lock_internal(cp_multimap *map, int type)
{
    int rc;

    switch (type)
    {
        case COLLECTION_LOCK_READ:
            rc = cp_lock_rdlock(map->lock);
            break;

        case COLLECTION_LOCK_WRITE:
            rc = cp_lock_wrlock(map->lock);
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

static int cp_multimap_unlock_internal(cp_multimap *map)
{
    return cp_lock_unlock(map->lock);
}

int cp_multimap_txlock(cp_multimap *map, int type)
{
    /* clear errno to allow client code to distinguish between a NULL return
     * value indicating the map doesn't contain the requested value and NULL
     * on locking failure in map operations
     */
    if (map->mode & COLLECTION_MODE_NOSYNC) return 0;
    if ((map->mode & COLLECTION_MODE_IN_TRANSACTION) && 
        map->txtype == COLLECTION_LOCK_WRITE)
    {
        cp_thread self = cp_thread_self();
        if (cp_thread_equal(self, map->txowner)) return 0;
    }
    errno = 0;
    return cp_multimap_lock_internal(map, type);
}

int cp_multimap_txunlock(cp_multimap *map)
{
    if (map->mode & COLLECTION_MODE_NOSYNC) return 0;
    if ((map->mode & COLLECTION_MODE_IN_TRANSACTION) && 
        map->txtype == COLLECTION_LOCK_WRITE)
    {
        cp_thread self = cp_thread_self();
        if (cp_thread_equal(self, map->txowner)) return 0;
    }
    return cp_multimap_unlock_internal(map);
}

/* lock and set the transaction indicators */
int cp_multimap_lock(cp_multimap *map, int type)
{
    int rc;
    if ((map->mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
    if ((rc = cp_multimap_lock_internal(map, type))) return rc;
    map->txtype = type;
    map->txowner = cp_thread_self();
    map->mode |= COLLECTION_MODE_IN_TRANSACTION;
    return 0;
}

/* unset the transaction indicators and unlock */
int cp_multimap_unlock(cp_multimap *map)
{
    cp_thread self = cp_thread_self();
    if (map->txowner == self)
    {
        map->txowner = 0;
        map->txtype = 0;
        map->mode ^= COLLECTION_MODE_IN_TRANSACTION;
    }
    else if (map->txtype == COLLECTION_LOCK_WRITE)
        return -1;
    return cp_multimap_unlock_internal(map);
}

/* set mode bits on the map mode indicator */
int cp_multimap_set_mode(cp_multimap *map, int mode)
{
    int rc;
    int nosync; 

    /* can't set NOSYNC in the middle of a transaction */
    if ((map->mode & COLLECTION_MODE_IN_TRANSACTION) && 
        (mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
    /* COLLECTION_MODE_MULTIPLE_VALUES must be set at creation time */    
    if ((mode & COLLECTION_MODE_MULTIPLE_VALUES)) return EINVAL;

    if ((rc = cp_multimap_txlock(map, COLLECTION_LOCK_WRITE))) return rc;
    
    nosync = map->mode & COLLECTION_MODE_NOSYNC;

    map->mode |= mode;

    if (!nosync)
        cp_multimap_txunlock(map);

    return 0;
}

/* unset mode bits on the map mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the map was not previously synchronized, the 
 * internal synchronization structure is initialized.
 */
int cp_multimap_unset_mode(cp_multimap *map, int mode)
{
    int rc;
    int nosync;

    /* COLLECTION_MODE_MULTIPLE_VALUES can't be unset */
    if ((mode & COLLECTION_MODE_MULTIPLE_VALUES)) return EINVAL;

    if ((rc = cp_multimap_txlock(map, COLLECTION_LOCK_WRITE))) return rc;
    
    nosync = map->mode & COLLECTION_MODE_NOSYNC;

    /* handle the special case of unsetting COLLECTION_MODE_NOSYNC */
    if ((mode & COLLECTION_MODE_NOSYNC) && map->lock == NULL)
    {
        /* map can't be locked in this case, no need to unlock on failure */
        if ((map->lock = malloc(sizeof(cp_lock))) == NULL)
            return -1;
        if (cp_lock_init(map->lock, NULL))
            return -1;
    }
    
    /* unset specified bits */
    map->mode &= map->mode ^ mode;
    if (!nosync)
        cp_multimap_txunlock(map);

    return 0;
}

int cp_multimap_get_mode(cp_multimap *map)
{
    return map->mode;
}


static cp_index_map_node *sibling(cp_index_map_node *node)
{
    return (node->up != NULL ) && (node == node->up->left) ? 
		node->up->right : node->up->left;
}

static int is_right_child(cp_index_map_node *node)
{
    return (node->up->right == node);
}

static int is_left_child(cp_index_map_node *node)
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
static void left_rotate(cp_index_map *tree, cp_index_map_node *p)
{
    cp_index_map_node *q = p->right;
    cp_index_map_node **sup;
    
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
static void right_rotate(cp_index_map *tree, cp_index_map_node *p)
{
    cp_index_map_node *q = p->left;
    cp_index_map_node **sup;
    
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
static void rebalance(cp_index_map *tree, cp_index_map_node *node)
{
    cp_index_map_node *up = node->up;
    if (up == NULL || up->color == MM_BLACK) return;
    if (sibling(up) && sibling(up)->color == MM_RED)
    {
        up->color = MM_BLACK;
        sibling(up)->color = MM_BLACK;
        if (up->up->up)
        {
            up->up->color = MM_RED;
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

        node->up->color = MM_BLACK;
        node->up->up->color = MM_RED;

        if (is_left_child(node)) 
            right_rotate(tree, node->up->up);
        else 
            left_rotate(tree, node->up->up);
    }
}

void cp_index_map_drop(cp_index_map *tree, void *entry);

static void 
	multimap_swap_secondaries(cp_multimap *map, void *before, void *after)
{
	if (map->index)
	{
		cp_hashlist_iterator itr;
		if (cp_hashlist_iterator_init(&itr, map->index, 
									  COLLECTION_LOCK_NONE) == 0)
		{
			cp_index_map *curr;
			while ((curr = (cp_index_map *) 
					        cp_hashlist_iterator_next_value(&itr)))
				cp_index_map_drop(curr, before);
			cp_hashlist_iterator_release(&itr);
		}
	}
}

/* update_index_map_node - implement COLLECTION_MODE_DEEP and 
 * COLLECTION_MODE_MULTIPLE_VALUES when inserting a * value for an existing key
 */
static void *
    update_index_map_node(cp_index_map *tree, 
                          cp_index_map_node *node, 
                          void *entry)
{
	void *before = node->entry;
    void *new_entry = entry;
        
    if (!(tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
        node->entry = new_entry;
    else
    {
        cp_vector_add_element(node->entry, new_entry);
        new_entry = node->entry;
    }

	/* if this is the primary index, we'll have to iterate through all other
	 * indices to swap pointers to the new value
	 */
	if (tree->owner->primary == tree && new_entry != before)
		multimap_swap_secondaries(tree->owner, before, new_entry);

    if (tree->mode & COLLECTION_MODE_DEEP)
    {
        if (tree->owner->dtr && !(tree->mode & COLLECTION_MODE_MULTIPLE_VALUES))
            (*tree->owner->dtr)(before);
    }

    return new_entry;
}

int cp_index_compare_internal(cp_index *index, void *a, void *b)
{
	void *e;
	if (index->type == CP_UNIQUE)
		e = a;
	else
	{
		cp_vector *v = a;
		if (v && cp_vector_size(v))
			e = cp_vector_element_at(v, 0);
#ifdef DEBUG
		else
		{
			DEBUGMSG("empty multiple value node %p", a);
		}
#endif
	}

	return index->key ? 
		(*index->cmp)((*index->key)(e), (*index->key)(b)) : 
		(*index->cmp)(e, b);
}


/*
 * cp_index_map_insert iterates through the tree, finds where the new node fits
 * in, puts it there, then calls rebalance. 
 *
 * If a mapping for the given key already exists it is replaced unless 
 * COLLECTION_MODE_MULTIPLE_VALUES is set, is which case a new mapping is 
 * added. By default COLLECTION_MODE_MULTIPLE_VALUES is not set.
 */
void *cp_index_map_insert(cp_index_map *tree, void *entry)
{
    void *res = NULL;

    if (tree->root == NULL)
    {
        tree->root = create_index_map_node(tree, entry);
        if (tree->root == NULL) 
		{
			errno = CP_MEMORY_ALLOCATION_FAILURE;
			goto DONE;
		}
        res = tree->root->entry;
        tree->root->color = MM_BLACK;
    }
    else
    {
        int cmp;
        cp_index_map_node **curr = &tree->root;
        cp_index_map_node *prev = NULL;

        while (*curr)
        {
            prev = *curr;
            cmp = cp_index_compare_internal(tree->index, (*curr)->entry, entry);
            if (cmp > 0)
                curr = &(*curr)->left;
            else if (cmp < 0) 
                curr = &(*curr)->right;
            else /* replace */
            {
                res = update_index_map_node(tree, *curr, entry);
                break;
            }
        }

        if (*curr == NULL) /* not replacing, create new node */
        {
            *curr = create_index_map_node(tree, entry);
            if (*curr == NULL) goto DONE;
            res = (*curr)->entry;
            (*curr)->up = prev;
            rebalance(tree, *curr);
        }
    }

DONE:
    return res;
}

void *cp_index_map_get(cp_index_map *tree, void *entry);

int cp_multimap_match_unique(cp_multimap *map, 
						     void *entry, 
						   	 cp_index **index, 
						   	 void **existing)
{							 
	void *res = NULL;
	int count = 0;
	cp_index_map *idx;
	cp_hashlist_iterator itr;

	/* try default index first */
	if (map->default_map->index->type == CP_UNIQUE)
		if ((res = cp_multimap_get(map, entry))) 
	{
		if (index) *index = map->default_map->index;
		if (existing) *existing = res;
		count = 1;
	}

	cp_hashlist_iterator_init(&itr, map->index, COLLECTION_LOCK_NONE);
	while ((idx = (cp_index_map *) cp_hashlist_iterator_next_value(&itr)))
	{
		if (idx->index->type == CP_UNIQUE)
			if ((res = cp_index_map_get(idx, entry)))
			{
				if (count == 0)
				{
					if (index) *index = idx->index;
					if (existing) *existing = res;
				}
				count++;
			}
	}
	cp_hashlist_iterator_release(&itr);

	return count;
}

void *cp_index_map_delete(cp_index_map *tree, void *entry);

void *cp_multimap_insert(cp_multimap *map, void *entry, int *err)
{
    void *res = NULL;
    void *dupl = NULL;
    cp_hashlist_iterator *itr = NULL;
    cp_hashlist_entry *idx;

    if (cp_multimap_txlock(map, COLLECTION_LOCK_WRITE)) 
	{
		if (err != NULL) *err = CP_LOCK_FAILED;
		return NULL;
	}

	if (cp_multimap_match_unique(map, entry, NULL, &res))
	{
		if (err != NULL) *err = CP_UNIQUE_INDEX_VIOLATION;
		goto DONE;
	}

	if ((map->mode & COLLECTION_MODE_COPY))
	{
        dupl = map->copy ? (*map->copy)(entry) : entry;
        if (dupl)
			entry = dupl;
		else
			goto INSERT_ERROR;
	}

    /* add entry on primary index */
    if ((res = cp_index_map_insert(map->primary, entry)) == NULL)
	{
		if (err != NULL) *err = CP_MEMORY_ALLOCATION_FAILURE;
        goto DONE;
	}

    /* add entry on secondary indices */
    itr = cp_hashlist_create_iterator(map->index, COLLECTION_MODE_NOSYNC);
    if (itr == NULL) 
	{
		if (err != NULL) *err = errno;
	    map->items++;
		goto DONE;
	}

    while (idx = cp_hashlist_iterator_next(itr))
        if (cp_index_map_insert(idx->value, res) == NULL)
		{
			if (err != NULL) *err = CP_MEMORY_ALLOCATION_FAILURE;
            goto INSERT_ERROR;
		}

    map->items++;
    goto DONE;


INSERT_ERROR:
    if (itr)
    {
        cp_hashlist_entry *idx;
        while (idx = cp_hashlist_iterator_prev(itr))
            cp_index_map_drop(idx->value, entry);
    }
	cp_index_map_delete(map->primary, entry);
	if ((dupl != NULL) && (dupl != entry) && map->dtr)
		(*map->dtr)(dupl);

DONE:
    if (itr) cp_hashlist_iterator_destroy(itr);
    cp_multimap_txunlock(map);
    return res;
}

/* cp_index_map_reindex() repositions an entry being edited in a way that 
 * modifies its value in respect to the index maintained by this index map. 
 * This function is called by cp_multimap_reindex. 
 */
int cp_index_map_reindex(cp_index_map *tree, void *before, void *after)
{
	int rc = 0;
	int cmp;
	cp_index_map_node **curr;
	cp_index_map_node *prev;

    if (tree->root == NULL) 
    {
#ifdef DEBUG
		DEBUGMSG("can\'t reindex entry at %p", before);
#endif
		return -1;
	}

	/* remove old entry */
	cp_index_map_drop(tree, before);
    if (tree->root == NULL)
    {
        tree->root = create_index_map_node(tree, after);
        if (tree->root == NULL) 
		{
#ifdef DEBUG
			cp_error(CP_MEMORY_ALLOCATION_FAILURE, 
					 "can\'t reindex entry at %p", before);
#endif
			return -1;
		}
        tree->root->color = MM_BLACK;
		return 0;
    }

	/* perform insertion */
    curr = &tree->root;
    prev = NULL;
    while (*curr)
    {
        prev = *curr;
        cmp = cp_index_compare_internal(tree->index, (*curr)->entry, after);
        if (cmp > 0)
            curr = &(*curr)->left;
        else if (cmp < 0) 
            curr = &(*curr)->right;
        else /* replace */
        {
			if ((tree->mode & COLLECTION_MODE_MULTIPLE_VALUES) == 0)
				(*curr)->entry = after;
			else
				cp_vector_add_element((*curr)->entry, after);
            break;
        }
    }

    if (*curr == NULL) /* not replacing, create new node */
    {
        *curr = create_index_map_node(tree, after);
        if (*curr == NULL) 
		{
#ifdef DEBUG
			int err = errno;
			cp_perror(CP_MEMORY_ALLOCATION_FAILURE, err, 
					  "can\'t create new node reindexing %p", before);
#endif
			return -1;
		}
        (*curr)->up = prev;
        rebalance(tree, *curr);
    }

	return 0;
}

/* determine which indices need to be refreshed, then swap ``after'' and 
 * ``before''
 */
int cp_multimap_reindex(cp_multimap *map, void *before, void *after)
{
	int rc;
	void *existing;
	void *dupl = NULL;
	cp_index *index;
	int violation_count;

    if (cp_multimap_txlock(map, COLLECTION_LOCK_WRITE)) return CP_LOCK_FAILED;

	violation_count = cp_multimap_match_unique(map, after, &index, &existing);
	if (violation_count > 2 || 
		(violation_count == 2 && existing != before))
	{
		printf("unique index violation. ciao.\n");
		*(char *) 0 = 0;
		return CP_UNIQUE_INDEX_VIOLATION;
	}

	if ((map->mode & COLLECTION_MODE_COPY))
	{
        dupl = map->copy ? (*map->copy)(after) : after;
        if (dupl)
			after = dupl;
		else
			goto REINDEX_ERROR;
	}

	if (map->index)
	{
		cp_hashlist_iterator itr;
		if (cp_hashlist_iterator_init(&itr, map->index, 
									  COLLECTION_LOCK_NONE) == 0)
		{
			cp_index_map *curr;
			while ((curr = (cp_index_map *) 
						    cp_hashlist_iterator_next_value(&itr)))
			{
					cp_index_map_reindex(curr, before, after);
			}
			cp_hashlist_iterator_release(&itr);
		}
	}

//	if ((map->mode & COLLECTION_MODE_MULTIPLE_VALUES))
//		cp_index_map_reindex(map->default_map, before, after);

//	if ((map->mode & COLLECTION_MODE_MULTIPLE_VALUES))
	cp_index_map_reindex(map->primary, before, after);

	goto REINDEX_DONE;

REINDEX_ERROR:
	if (dupl && after != dupl && map->dtr) 
		(*map->dtr)(dupl);

REINDEX_DONE:
    cp_multimap_txunlock(map);

	return 0;
}

/* cp_index_map_get - return the value mapped to the given key or NULL if none
 * is found. If COLLECTION_MODE_MULTIPLE_VALUES is set the returned value is a
 * cp_vector object or NULL if no mapping is found. 
 */
void *cp_index_map_get(cp_index_map *tree, void *entry)
{
    cp_index_map_node *curr;
    void *res = NULL;

    curr = tree->root;
    while (curr)
    {
        int c = cp_index_compare_internal(tree->index, curr->entry, entry);
        if (c == 0) return curr->entry;
        curr = (c > 0) ? curr->left : curr->right;
    }

    if (curr) res = curr->entry;

    return res;
}
    
void *cp_multimap_get(cp_multimap *map, void *entry)
{
    void *res = NULL;

    if (cp_multimap_txlock(map, COLLECTION_LOCK_READ)) return NULL;
    res = cp_index_map_get(map->default_map, entry); 
    cp_multimap_txunlock(map);

    return res;
}

void *cp_index_map_find(cp_index_map *tree, void *entry, cp_op op)
{
	int cmp;
	cp_index_map_node **curr;
	cp_index_map_node *prev;
	cp_index_map_node *res = NULL;

	if (tree->root == NULL) return NULL;

	curr = &tree->root;
	while (*curr)
	{
		prev = *curr;
		cmp = cp_index_compare_internal(tree->index, (*curr)->entry, entry);

		if (cmp == 0) break;
		if (op == CP_OP_NE)
		{
			res = *curr;
			goto FIND_DONE;
		}

		if (cmp > 0)
			curr = &(*curr)->left;
		else 
			curr = &(*curr)->right;
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
	return res ? res->entry : NULL;
}

void *cp_multimap_find(cp_multimap *map, void *entry, cp_op op)
{
    void *res = NULL;

    if (cp_multimap_txlock(map, COLLECTION_LOCK_READ)) return NULL;
    res = cp_index_map_find(map->default_map, entry, op); 
    cp_multimap_txunlock(map);

    return res;
}

void *cp_multimap_find_by_index(cp_multimap *map, 
								cp_index *index, 
								void *entry, 
								cp_op op)
{
    void *res = NULL;

	cp_index_map *tree = cp_hashlist_get(map->index, index);
	if (tree == NULL)
	{
#ifdef DEBUG
        cp_error(CP_INVALID_VALUE, 
            "request for search by unknown index %p", index);
#endif
		return NULL;
	}

    if (cp_multimap_txlock(map, COLLECTION_LOCK_READ)) return NULL;
    res = cp_index_map_find(tree, entry, op); 
    cp_multimap_txunlock(map);

    return res;
}


void *cp_multimap_get_by_index(cp_multimap *map, cp_index *index, void *entry)
{
    void *res = NULL;
    cp_index_map *tree;
    if (index == NULL) return cp_multimap_get(map, entry);
    
    if (cp_multimap_txlock(map, COLLECTION_LOCK_READ)) return NULL;

#ifdef DEBUG
    if (map->index == NULL)
    {
        DEBUGMSG("no secondary indices set on multimap");
        return NULL;
    }
#endif /* DEBUG */

    tree = cp_hashlist_get(map->index, index);
    if (tree)
        res = cp_index_map_get(tree, entry);
#ifdef DEBUG
    else
        cp_error(CP_INVALID_VALUE, 
            "request for lookup by unknown index %p", index);
#endif
    cp_multimap_txunlock(map);

    return res;
}

int cp_index_map_contains(cp_index_map *tree, void *entry)
{
    return (cp_index_map_get(tree, entry) != NULL);
}

/* return non-zero if entry could be found on any index */
CPROPS_DLL
int cp_multimap_contains(cp_multimap *map, void *entry)
{
	int res = 0;
	cp_index_map *idx;
	cp_hashlist_iterator itr;

	/* try default index first */
	if (cp_multimap_get(map, entry) != NULL) return 1;

	cp_hashlist_iterator_init(&itr, map->index, COLLECTION_LOCK_NONE);
	while ((idx = (cp_index_map *) cp_hashlist_iterator_next_value(&itr)))
	{
		if (cp_index_map_get(idx, entry) != NULL)
		{
			res = 1;
			break;
		}
	}
	cp_hashlist_iterator_release(&itr);

	return res;
}


/* helper function for deletion */
static void swap_node_content(cp_index_map_node *a, cp_index_map_node *b)
{
    void *tmp;

    tmp = a->entry;
    a->entry = b->entry;
    b->entry = tmp;
}

/*
 * helper function for cp_index_map_delete to remove nodes with either a left 
 * NULL branch or a right NULL branch
 */
static void index_map_unlink(cp_index_map *tree, cp_index_map_node *node)
{
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
static void delete_rebalance(cp_index_map *tree, cp_index_map_node *n)
{
    if (n->up)
    {
        cp_index_map_node *sibl = sibling(n);

        if (sibl->color == MM_RED)
        {
            n->up->color = MM_RED;
            sibl->color = MM_BLACK;
            if (is_left_child(n))
                left_rotate(tree, n->up);
            else
                right_rotate(tree, n->up);
            sibl = sibling(n);
        }

        if (n->up->color == MM_BLACK &&
            sibl->color == MM_BLACK &&
            (sibl->left == NULL || sibl->left->color == MM_BLACK) &&
            (sibl->right == NULL || sibl->right->color == MM_BLACK))
        {
            sibl->color = MM_RED;
            delete_rebalance(tree, n->up);
        }
        else
        {
            if (n->up->color == MM_RED &&
                sibl->color == MM_BLACK &&
                (sibl->left == NULL || sibl->left->color == MM_BLACK) &&
                (sibl->right == NULL || sibl->right->color == MM_BLACK))
            {
                sibl->color = MM_RED;
                n->up->color = MM_BLACK;
            }
            else
            {
                if (is_left_child(n) && 
                    sibl->color == MM_BLACK &&
                    sibl->left && sibl->left->color == MM_RED && 
                    (sibl->right == NULL || sibl->right->color == MM_BLACK))
                {
                    sibl->color = MM_RED;
                    sibl->left->color = MM_BLACK;
                    right_rotate(tree, sibl);
                    
                    sibl = sibling(n);
                }
                else if (is_right_child(n) &&
                    sibl->color == MM_BLACK &&
                    sibl->right && sibl->right->color == MM_RED &&
                    (sibl->left == NULL || sibl->left->color == MM_BLACK))
                {
                    sibl->color = MM_RED;
                    sibl->right->color = MM_BLACK;
                    left_rotate(tree, sibl);

                    sibl = sibling(n);
                }

                sibl->color = n->up->color;
                n->up->color = MM_BLACK;
                if (is_left_child(n))
                {
                    sibl->right->color = MM_BLACK;
                    left_rotate(tree, n->up);
                }
                else
                {
                    sibl->left->color = MM_BLACK;
                    right_rotate(tree, n->up);
                }
            }
        }
    }
}

/* cp_index_map_delete - deletes the value mapped to the given key from the 
 * tree and returns the value removed. 
 */
void *cp_index_map_delete(cp_index_map *tree, void *entry)
{
    void *res = NULL;
    cp_index_map_node *node; 
    int cmp;

    node = tree->root;
    while (node)
    {
        cmp = cp_index_compare_internal(tree->index, node->entry, entry);
        if (cmp > 0)
            node = node->left;
        else if (cmp < 0)
            node = node->right;
        else /* found */
            break;
    }

    if (node) /* may be null if not found */
    {
        cp_index_map_node *child; 
        res = node->entry;

        if (node->right && node->left)
        {
            cp_index_map_node *surrogate = node;
            node = node->right;
            while (node->left) node = node->left;
            swap_node_content(node, surrogate);
        }
        child = node->right ? node->right : node->left;

        /* if the node was red - no rebalancing required */
        if (node->color == MM_BLACK)
        {
            if (child)
            {
                /* single red child - paint it black */
                if (child->color == MM_RED)
                    child->color = MM_BLACK; /* and the balance is restored */
                else
                    delete_rebalance(tree, child);
            }
            else 
                delete_rebalance(tree, node);
        }

        index_map_unlink(tree, node);
        cp_index_map_destroy_node(tree, node);
    }

    return res;
}

/* drop a single entry. Locate the entry, if it's a multiple mapping iterate
 * over multiple values and remove the specified entry only. If operating on
 * the default index, implement mode settings, ie destroy node if required.
 */
void cp_index_map_drop(cp_index_map *tree, void *entry)
{
    cp_index_map_node *node; 
    int cmp;

    node = tree->root;
    while (node)
    {
        cmp = cp_index_compare_internal(tree->index, node->entry, entry);
        if (cmp > 0)
            node = node->left;
        else if (cmp < 0)
            node = node->right;
        else /* found */
            break;
    }

    if (node) /* may be null if not found */
    {
        cp_index_map_node *child; 

        /* find the single entry to be removed */
        if (tree->index->type == CP_MULTIPLE)
        {
            int i;
            cp_vector *v = node->entry;
            int size = cp_vector_size(v);

            for (i = 0; i < size; i++)
                if (cp_vector_element_at(v, i) == entry)
                {
                    cp_vector_remove_element_at(v, i);
                    break;
                }

#ifdef DEBUG
			if (i == size) 
			{
				DEBUGMSG("can\'t find item to remove: entry %p", entry);
				*(char *) 0 = 1;
			}
#endif

            /* still more multiple values in this vector, safe to return */
            if (cp_vector_size(v)) return;
        }

        if (node->right && node->left)
        {
            cp_index_map_node *surrogate = node;
            node = node->right;
            while (node->left) node = node->left;
            swap_node_content(node, surrogate);
        }
        child = node->right ? node->right : node->left;

        /* if the node was red - no rebalancing required */
        if (node->color == MM_BLACK)
        {
            if (child)
            {
                /* single red child - paint it black */
                if (child->color == MM_RED)
                    child->color = MM_BLACK; /* and the balance is restored */
                else
                    delete_rebalance(tree, child);
            }
            else 
                delete_rebalance(tree, node);
        }

        index_map_unlink(tree, node);
//		cp_index_map_drop_node(tree, node);        
		cp_index_map_destroy_node(tree, node);        
    }
}

/* drop the given entry from all indices */
void cp_multimap_drop(cp_multimap *map, 
                      cp_index *index, 
                      void *entry)
{
    cp_hashlist_iterator *itr = NULL;

    if (map->index) 
        itr = cp_hashlist_create_iterator(map->index, COLLECTION_LOCK_NONE);

    if (itr)
    {
        cp_hashlist_entry *idx;

        while (idx = cp_hashlist_iterator_next(itr))
            if (idx->key != index && idx->key != &map->primary) 
				cp_index_map_drop(idx->value, entry);

		cp_hashlist_iterator_destroy(itr);
    }

	if ((map->mode & COLLECTION_MODE_MULTIPLE_VALUES))
        cp_index_map_drop(map->default_map, entry);

    if (index)
        cp_index_map_drop(map->primary, entry);
}

/* delete the entry (or entries) matching the given value by the tree primary
 * key. 
 */
void *cp_multimap_remove(cp_multimap *map, void *entry)
{
    void *res = NULL;
    int count = 0;

    if (cp_multimap_txlock(map, COLLECTION_LOCK_WRITE)) return NULL;

    if (map->mode & COLLECTION_MODE_MULTIPLE_VALUES)
    {
        int i;
        cp_vector *v = cp_index_map_get(map->default_map, entry);
        if (v)
        {
			res = v;
            count = cp_vector_size(v);
            for (i = 0; i < count; i++)
			{
				void *element = cp_vector_element_at(v, i);
                cp_multimap_drop(map, NULL, element);
				cp_index_map_delete(map->primary, element);
			}
        }
    }
    else
    {
        void *e = cp_index_map_get(map->primary, entry);
        if (e)
        {
			res = e;
            cp_multimap_drop(map, NULL, e);
        	cp_index_map_delete(map->primary, e);
            count = 1;
        }
    }

    if (count)
        map->items -= count;

    cp_multimap_txunlock(map);

    return res;
}

/* the point in deleting entries by index is when you have multiple values for
 * a given entry respective to the given index, and you want to delete them all
 * in one go.
 */
void *cp_multimap_remove_by_index(cp_multimap *map, 
                                  cp_index *index, 
                                  void *entry)
{
    void *res = NULL;
    cp_index_map *lookup;
    int count = 0;

    if (index == NULL) 
        return cp_multimap_remove(map, entry);

    if (cp_multimap_txlock(map, COLLECTION_LOCK_WRITE)) return NULL;
    
	if (index != map->primary->index)
	    lookup = cp_hashlist_get(map->index, index);
	else
		lookup = map->primary;

#ifdef DEBUG
    if (lookup == NULL)
    {
        cp_error(CP_INVALID_VALUE, 
                 "request for removal by unknown index %p", index);
        goto DELETE_DONE;
    }
#endif

    if (lookup->mode & COLLECTION_MODE_MULTIPLE_VALUES)
    {
        int i;
        cp_vector *v = cp_index_map_get(lookup, entry);
		if (v)
		{
			res = v;
	        count = cp_vector_size(v);
    	    for (i = 0; i < count; i++)
        	    cp_multimap_drop(map, index, cp_vector_element_at(v, i));
		}
    }
    else
    {
        void *e = cp_index_map_get(lookup, entry);
		if (e)
		{
			cp_index *drop_index = (lookup == map->primary) ? NULL : drop_index;
			res = e;
	        cp_multimap_drop(map, drop_index, e);
    	    count = 1;
		}
    }

    if (count)
    {
        cp_index_map_delete(lookup, entry);
        map->items -= count;
    }
    
#ifdef DEBUG
DELETE_DONE:
#endif
    cp_multimap_txunlock(map);

    return res;
}

static int 
    index_map_scan_pre_order(cp_index_map_node *node, 
                             cp_callback_fn callback, 
                             void *prm)
{
    int rc;
    
    if (node) 
    {
        if ((rc = (*callback)(node, prm))) return rc;
        if ((rc = index_map_scan_pre_order(node->left, callback, prm))) return rc;
        if ((rc = index_map_scan_pre_order(node->right, callback, prm))) return rc;
    }

    return 0;
}

int cp_multimap_callback_preorder(cp_multimap *map, 
                                  cp_callback_fn callback, 
                                  void *prm)
{
    int rc;

    if ((rc = cp_multimap_txlock(map, COLLECTION_LOCK_READ))) return rc;
    rc = index_map_scan_pre_order(map->default_map->root, callback, prm);
    cp_multimap_txunlock(map);

    return rc;
}

static int 
    index_map_scan_in_order(cp_index_map_node *node, 
                            cp_callback_fn callback, 
                            void *prm)
{
    int rc;
    
    if (node) 
    {
        if ((rc = index_map_scan_in_order(node->left, callback, prm))) return rc;
        if ((rc = (*callback)(node, prm))) return rc;
        if ((rc = index_map_scan_in_order(node->right, callback, prm))) return rc;
    }

    return 0;
}

int cp_multimap_callback(cp_multimap *map, cp_callback_fn callback, void *prm)
{
    int rc;

    if ((rc = cp_multimap_txlock(map, COLLECTION_LOCK_READ))) return rc;
    rc = index_map_scan_in_order(map->default_map->root, callback, prm);
    cp_multimap_txunlock(map);

    return rc;
}

int cp_multimap_callback_by_index(cp_multimap *map, 
                                  cp_index *index, 
								  cp_callback_fn callback, 
								  void *prm)
{
    int rc;
	cp_index_map *imap;

	if (index == NULL) return cp_multimap_callback(map, callback, prm);

    if ((rc = cp_multimap_txlock(map, COLLECTION_LOCK_READ))) return rc;

	imap = cp_hashlist_get(map->index, index);
	if (imap == NULL) 
		rc = -1;
	else
	    rc = index_map_scan_in_order(imap->root, callback, prm);
    cp_multimap_txunlock(map);

    return rc;
}



static int 
    index_map_scan_post_order(cp_index_map_node *node, cp_callback_fn callback, void *prm)
{
    int rc;
    
    if (node) 
    {
        if ((rc = index_map_scan_post_order(node->left, callback, prm))) return rc;
        if ((rc = index_map_scan_post_order(node->right, callback, prm))) return rc;
        if ((rc = (*callback)(node, prm))) return rc;
    }

    return 0;
}

int cp_multimap_callback_postorder(cp_multimap *map, 
                                 cp_callback_fn callback, 
                                 void *prm)
{
    int rc;

    if ((rc = cp_multimap_txlock(map, COLLECTION_LOCK_READ))) return rc;
    rc = index_map_scan_post_order(map->default_map->root, callback, prm);
    cp_multimap_txunlock(map);

    return rc;
}

int cp_multimap_count(cp_multimap *tree)
{
    return tree->items;
}


void cp_index_map_node_print(cp_index_map_node *node, int level)
{
    int i;
    if (node->right) cp_index_map_node_print(node->right, level + 1);
    for (i = 0; i < level; i++) printf("  . ");
    printf("(%d) [%s]\n", node->color, (char *) node->entry);
    if (node->left) cp_index_map_node_print(node->left, level + 1);
}

void cp_index_map_node_multi_print(cp_index_map_node *node, int level)
{
    int i;
    cp_vector *v = node->entry;
    if (node->right) cp_index_map_node_multi_print(node->right, level + 1);
    
    for (i = 0; i < level; i++) printf("  . ");
    printf("(%d) [ ", node->color);

    for (i = 0; i < cp_vector_size(v); i++)
        printf("%s; ", (char *) cp_vector_element_at(v, i));

    printf("]\n");

    if (node->left) cp_index_map_node_multi_print(node->left, level + 1);
}

void cp_multimap_dump(cp_multimap *map)
{
    if (map->default_map->root) 
    {
        if ((map->mode & COLLECTION_MODE_MULTIPLE_VALUES))
            cp_index_map_node_multi_print(map->default_map->root, 0);
        else
            cp_index_map_node_print(map->default_map->root, 0);
    }
}

/* set map to use given mempool or allocate a new one if pool is NULL */
int cp_multimap_use_mempool(cp_multimap *map, cp_mempool *pool)
{
    int rc = 0;
    
    if ((rc = cp_multimap_txlock(map, COLLECTION_LOCK_WRITE))) return rc;
    
    if (pool)
    {
        if (pool->item_size < sizeof(cp_index_map_node))
        {
            rc = EINVAL;
            goto DONE;
        }
        if (map->mempool) 
        {
            if (map->items) 
            {
                rc = ENOTEMPTY;
                goto DONE;
            }
            cp_mempool_destroy(map->mempool);
        }
        cp_mempool_inc_refcount(pool);
        map->mempool = pool;
    }
    else
    {
        map->mempool = 
            cp_mempool_create_by_option(COLLECTION_MODE_NOSYNC, 
                                        sizeof(cp_index_map_node), 0);
        if (map->mempool == NULL) 
        {
            rc = ENOMEM;
            goto DONE;
        }
    }

DONE:
    cp_multimap_txunlock(map);
    return rc;
}


/* set map to use a shared memory pool */
int cp_multimap_share_mempool(cp_multimap *map, cp_shared_mempool *pool)
{
    int rc;

    if ((rc = cp_multimap_txlock(map, COLLECTION_LOCK_WRITE))) return rc;

    if (map->mempool)
    {
        if (map->items)
        {
            rc = ENOTEMPTY;
            goto DONE;
        }

        cp_mempool_destroy(map->mempool);
    }

    map->mempool = cp_shared_mempool_register(pool, sizeof(cp_index_map_node));
    if (map->mempool == NULL) 
    {
        rc = ENOMEM;
        goto DONE;
    }
    
DONE:
    cp_multimap_txunlock(map);
    return rc;
}

/* add an index */
cp_index *cp_multimap_create_index(cp_multimap *map, 
                                   cp_index_type type,
                                   cp_key_fn key, 
                                   cp_compare_fn cmp,
                                   int *err)
{
    int mode;
    cp_index *res = NULL;
    cp_index *desc;
    cp_index_map *index;
	int rc;

    if ((rc = cp_multimap_txlock(map, COLLECTION_LOCK_WRITE))) 
	{
		if (err != NULL) *err = rc;
        return NULL;
	}

    if (map->index == NULL)
    {
        map->index = 
            cp_hashlist_create_by_option(COLLECTION_MODE_NOSYNC |
                                         COLLECTION_MODE_DEEP, 3,
                                         cp_hash_addr, cp_hash_compare_addr, 
                                         NULL, NULL, 
                                         NULL, (cp_destructor_fn) free);
        if (map->index == NULL)
        {
            if (err != NULL) *err = CP_MEMORY_ALLOCATION_FAILURE;
            return NULL;
        }
    }
    else /* verify that no such index has been registered */
    {
        int exists = 0;
        cp_hashlist_entry *entry;
        cp_hashlist_iterator *itr = 
            cp_hashlist_create_iterator(map->index, COLLECTION_LOCK_NONE);

        while ((entry = cp_hashlist_iterator_next(itr)))
        {
            cp_index *curr = (cp_index *) entry->key;
            if (curr->key == key && curr->cmp == cmp)
            {
				res = curr;
                exists = 1;
                break;
            }
        }

        cp_hashlist_iterator_destroy(itr);

        if (exists)
        {
            if (err != NULL) *err = CP_ITEM_EXISTS;
            goto DONE;
        }
    }

    desc = calloc(1, sizeof(cp_index));
    if (desc == NULL) 
    {
        if (err != NULL) *err = CP_MEMORY_ALLOCATION_FAILURE;
        goto DONE;
    }
    desc->type = type;
    desc->key = key;
    desc->cmp = cmp;

    mode = map->mode & ~(COLLECTION_MODE_COPY | COLLECTION_MODE_DEEP);
    if (type == CP_UNIQUE)
        mode &= ~COLLECTION_MODE_MULTIPLE_VALUES;
    else
        mode |= COLLECTION_MODE_MULTIPLE_VALUES;

    index = cp_index_map_create(map, mode, desc);
    if (index == NULL) goto DONE;

    if (cp_hashlist_insert(map->index, desc, index)) res = desc;

DONE:
    cp_multimap_txunlock(map);
    return res;
}

