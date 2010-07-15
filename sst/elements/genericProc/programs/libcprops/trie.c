#include <stdlib.h>
#include "collection.h"
#include "trie.h"
#include "mtab.h"

/*
 * allocate a new node
 */
cp_trie_node *cp_trie_node_new(void *leaf, cp_mempool *pool) 
{ 
	cp_trie_node *node;
	if (pool) 
		node = (cp_trie_node *) cp_mempool_calloc(pool);
	else
		node = (cp_trie_node *) calloc(1, sizeof(cp_trie_node)); 
	if (node) 
	{ 
		node->others = mtab_new(0); 
		node->leaf = leaf; 
	} 

	return node; 
} 

/*
 * recursively delete the subtree at node
 */
void *cp_trie_node_delete(cp_trie *grp, cp_trie_node *node) 
{ 
	void *leaf = NULL; 

	if (node) 
	{ 
		mtab_delete_custom(node->others, grp, (mtab_dtr)cp_trie_delete_mapping);
		leaf = node->leaf; 
		if (grp->mempool)
			cp_mempool_free(grp->mempool, node);
		else
			free(node); 
	} 

	if (leaf && grp->delete_leaf && grp->mode & COLLECTION_MODE_DEEP)
		(*grp->delete_leaf)(leaf);

	return leaf; 
} 

/* 
 * uses the mtab_dtr 'owner' parameter (grp here) to recursively delete 
 * sub-nodes
 */
void cp_trie_delete_mapping(cp_trie *grp, mtab_node *map_node)
{
	if (map_node)
	{
		if (map_node->attr) free(map_node->attr);
		if (map_node->value) cp_trie_node_delete(grp, map_node->value);
	}
}

void cp_trie_node_unmap(cp_trie *grp, cp_trie_node **node) 
{ 
	cp_trie_node_delete(grp, *node); 
	*node = NULL; 
} 

int cp_trie_lock_internal(cp_trie *grp, int type)
{
	int rc = 0;
	switch(type)
	{
		case COLLECTION_LOCK_READ:
			rc = cp_lock_rdlock(grp->lock);
			break;

		case COLLECTION_LOCK_WRITE:
			rc = cp_lock_wrlock(grp->lock);
			break;

		case COLLECTION_LOCK_NONE:
		default:
			break; 
	}

	return rc;
}

int cp_trie_unlock_internal(cp_trie *grp)
{
	return cp_lock_unlock(grp->lock);
}

int cp_trie_txlock(cp_trie *grp, int type)
{
	if (grp->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (grp->mode & COLLECTION_MODE_IN_TRANSACTION && 
		grp->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, grp->txowner)) return 0;
	}
	return cp_trie_lock_internal(grp, type);
}

int cp_trie_txunlock(cp_trie *grp)
{
	if (grp->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (grp->mode & COLLECTION_MODE_IN_TRANSACTION && 
		grp->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, grp->txowner)) return 0;
	}
	return cp_trie_unlock_internal(grp);
}

/* lock and set the transaction indicators */
int cp_trie_lock(cp_trie *grp, int type)
{
	int rc;
	if ((grp->mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	if ((rc = cp_trie_lock_internal(grp, type))) return rc;
	grp->txtype = type;
	grp->txowner = cp_thread_self();
	grp->mode |= COLLECTION_MODE_IN_TRANSACTION;
	return 0;
}

/* unset the transaction indicators and unlock */
int cp_trie_unlock(cp_trie *grp)
{
	cp_thread self = cp_thread_self();
	if (grp->txowner == self)
	{
		grp->txtype = 0;
		grp->txowner = 0;
		grp->mode ^= COLLECTION_MODE_IN_TRANSACTION;
	}
	else if (grp->txtype == COLLECTION_LOCK_WRITE)
		return -1;
	return cp_trie_unlock_internal(grp);
}

/* get the current collection mode */
int cp_trie_get_mode(cp_trie *grp)
{
    return grp->mode;
}

/* set mode bits on the trie mode indicator */
int cp_trie_set_mode(cp_trie *grp, int mode)
{
	int nosync;

	/* can't set NOSYNC in the middle of a transaction */
	if ((grp->mode & COLLECTION_MODE_IN_TRANSACTION) && 
		(mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	
	nosync = grp->mode & COLLECTION_MODE_NOSYNC;
	if (!nosync)
		if (cp_trie_txlock(grp, COLLECTION_LOCK_WRITE))
			return -1;

	grp->mode |= mode;

	if (!nosync)
		cp_trie_txunlock(grp);

	return 0;
}

/* unset mode bits on the grp mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the grp was not previously synchronized, the 
 * internal synchronization structure is initalized.
 */
int cp_trie_unset_mode(cp_trie *grp, int mode)
{
	int nosync = grp->mode & COLLECTION_MODE_NOSYNC;

	if (!nosync)
		if (cp_trie_txlock(grp, COLLECTION_LOCK_WRITE))
			return -1;
	
	/* handle the special case of unsetting COLLECTION_MODE_NOSYNC */
	if ((mode & COLLECTION_MODE_NOSYNC) && grp->lock == NULL)
	{
		/* grp can't be locked in this case, no need to unlock on failure */
		if ((grp->lock = malloc(sizeof(cp_lock))) == NULL)
			return -1; 
		if (cp_lock_init(grp->lock, NULL))
			return -1;
	}
	
	/* unset specified bits */
    grp->mode &= grp->mode ^ mode;
	if (!nosync)
		cp_trie_txunlock(grp);

	return 0;
}


cp_trie *cp_trie_create_trie(int mode,
		                     cp_copy_fn copy_leaf,
							 cp_destructor_fn delete_leaf) 
{ 
	cp_trie *grp = calloc(1, sizeof(cp_trie)); 

	if (grp == NULL) return NULL; 
	grp->root = cp_trie_node_new(NULL, NULL); //~~ what if mempool set later?
	if (grp->root == NULL) 
	{ 
		free(grp); 
		return NULL; 
	} 

	if (!(mode & COLLECTION_MODE_NOSYNC))
	{
		if ((grp->lock = malloc(sizeof(cp_lock))) == NULL)
		{
			cp_trie_node_delete(grp, grp->root);
			free(grp);
			return NULL;
		}

		if (cp_lock_init(grp->lock, NULL)) 
		{
			free(grp->lock);
			cp_trie_node_delete(grp, grp->root);
			free(grp); 
			return NULL; 
		} 
	}

	grp->mode = mode;
	grp->copy_leaf = copy_leaf;
	grp->delete_leaf = delete_leaf;

	return grp; 
} 

cp_trie *cp_trie_create(int mode)
{
	return cp_trie_create_trie(mode, NULL, NULL);
}

/*
 * recursively deletes trie structure
 */
int cp_trie_destroy(cp_trie *grp) 
{ 
	int rc = 0; 

	/* no locking is done here. It is the application's responsibility to
	 * ensure that the trie isn't being destroyed while it's still in use
	 * by other threads.
	 */
	if (grp)
	{ 
		cp_trie_node_delete(grp, grp->root); 
		if (grp->lock) 
		{
			rc = cp_lock_destroy(grp->lock); 
			free(grp->lock);
		}
		free(grp); 
	} 
	else rc = -1; 

	return rc; 
} 

void *NODE_STORE(cp_trie_node *node, char *key, cp_trie_node *sub)
{
	char *k = strdup(key); 
	if (k == NULL) return NULL;
	return mtab_put(node->others, *k, sub, k);
}

/*
 * since cp_trie compresses single child nodes, the following cases are handled
 * here. 'abc' etc denote existing path, X marks the spot:
 *
 * (1) simple case: abc-X         - first mapping for abc
 *
 * (2) mid-branch:  ab-X-c        - abc already mapped, but ab isn't 
 *
 * (3) new branch:  ab-X-cd       - the key abcd is already mapped
 *                      \          
 *                       ef         the key abef is to be added
 *
 * (4) extending:   abc-de-X      - abc mapped, abcde isn't
 * 
 * (5) replace:     abc-X         - abc already mapped, just replace leaf  
 */
int cp_trie_node_store(cp_trie *grp, 
                       cp_trie_node *node, 
					   char *key, 
					   void *leaf) 
{ 

	char *next;
	mtab_node *map_node;
	cp_trie_node *sub;

	map_node = NODE_MATCH(node, key); 

	if (map_node == NULL) /* not mapped - just add it */
	{ 
		sub = cp_trie_node_new(leaf, grp->mempool); 
		if (NODE_STORE(node, key, sub) == NULL) return -1; 
	} 
	else
	{
		next = map_node->attr;
		while (*key && *key == *next) 
		{
			key++;
			next++;
		}
		
		if (*next) /* branch abc, key abx or ab */
		{
			cp_trie_node *old = map_node->value;
			if ((sub = cp_trie_node_new(NULL, grp->mempool)) == NULL) return -1;
			if (NODE_STORE(sub, next, old) == NULL) return -1;
			*next = '\0'; //~~ truncate and realloc - prevent dangling key
			map_node->value = sub;
			if (*key) /* key abx */
			{
				if ((NODE_STORE(sub, key, 
								cp_trie_node_new(leaf, grp->mempool)) == NULL))
					return -1;
			}
			else /* key ab */
				sub->leaf = leaf;
		}
		else if (*key) /* branch abc, key abcde */
			return cp_trie_node_store(grp, map_node->value, key, leaf);

		else  /* branch abc, key abc */
		{
			cp_trie_node *node = ((cp_trie_node *) map_node->value);
			if (node->leaf && grp->delete_leaf && 
				grp->mode & COLLECTION_MODE_DEEP)
				(*grp->delete_leaf)(node->leaf);
			node->leaf = leaf;
		}
	}
	return 0;
} 

/*
 * wrapper for the recursive insertion - implements collection mode setting
 */
int cp_trie_add(cp_trie *grp, char *key, void *leaf) 
{ 
	int rc = 0; 
	if ((rc = cp_trie_txlock(grp, COLLECTION_LOCK_WRITE))) return rc; 

	if ((grp->mode & COLLECTION_MODE_COPY) && grp->copy_leaf && (leaf != NULL))
	{
		leaf = (*grp->copy_leaf)(leaf);
		if (leaf == NULL) goto DONE;
	}

	if (key == NULL) /* map NULL key to root node */
	{
		if (grp->root->leaf && 
				grp->delete_leaf && grp->mode & COLLECTION_MODE_DEEP)
			(*grp->delete_leaf)(grp->root->leaf);
		grp->root->leaf = leaf; 
	}
	else 
	{
		if ((rc = cp_trie_node_store(grp, grp->root, key, leaf)))
			goto DONE;
	} 

	grp->path_count++; 

DONE:
	cp_trie_txunlock(grp);
	return rc; 
} 

/* helper functions for cp_trie_remove */

static char *mergestr(char *s1, char *s2)
{
	char *s;
	int len = strlen(s1) + strlen(s2);

	s = malloc((len + 1) * sizeof(char));
	if (s == NULL) return NULL;
	
#ifdef HAVE_STRLCPY
	strlcpy(s, s1, len + 1);
#else
	strcpy(s, s1);
#endif /* HAVE_STRLCPY */
#ifdef HAVE_STRLCAT
	strlcat(s, s2, len + 1);
#else
	strcat(s, s2);
#endif /* HAVE_STRLCAT */

	return s;
}

static mtab_node *get_single_entry(mtab *t)
{
	int i;

	for (i = 0; i < t->size; i++)
		if (t->table[i]) return t->table[i];

	return NULL;
}

/*
 * removing mappings, the following cases are possible:
 * 
 * (1) simple case:       abc-X       removing mapping abc
 *
 * (2) branch:            abc-de-X    removing mapping abcde -
 *                           \        mapping abcfg remains, but
 *                            fg      junction node abc no longer needed
 *
 * (3) mid-branch:        abc-X-de    removing mapping abc - mapping abcde
 *                                    remains
 */
int cp_trie_remove(cp_trie *grp, char *key, void **leaf)
{
	int rc = 0;
	cp_trie_node *link = grp->root; 
	cp_trie_node *prev = NULL;
	char ccurr, cprev = 0;
	mtab_node *map_node;
	char *branch;

	if (link == NULL) return 0; /* tree is empty */

	if ((rc = cp_trie_txlock(grp, COLLECTION_LOCK_READ))) return rc;
 
	if (key == NULL) /* as a special case, NULL keys are stored on the root */
	{
		if (link->leaf)
		{
			grp->path_count--;
			link->leaf = NULL;
		}
		goto DONE;
	}

	/* keep pointers one and two nodes up for merging in cases (2), (3) */
	ccurr = *key;
	while ((map_node = NODE_MATCH(link, key)) != NULL) 
	{
		branch = map_node->attr;
 
		while (*key && *key == *branch)
		{
			key++;
			branch++;
		}
		if (*branch) break; /* mismatch */
		if (*key == '\0') /* found */
		{
			char *attr;
			cp_trie_node *node = map_node->value;
			if (leaf) *leaf = node->leaf;
			if (node->leaf)
			{
				grp->path_count--;
				rc = 1;
				/* unlink leaf */
        		if (grp->delete_leaf && grp->mode & COLLECTION_MODE_DEEP)
                	(*grp->delete_leaf)(node->leaf);
				node->leaf = NULL;

				/* now remove node - case (1) */
				if (mtab_count(node->others) == 0) 
				{
					mtab_remove(link->others, ccurr);
					cp_trie_node_unmap(grp, &node);
					
					/* case (2) */
					if (prev &&
						mtab_count(link->others) == 1 && link->leaf == NULL)
					{
						mtab_node *sub2 = mtab_get(prev->others, cprev);
						mtab_node *sub = get_single_entry(link->others);
						attr = mergestr(sub2->attr, sub->attr);
						if (attr)
						{
							if (NODE_STORE(prev, attr, sub->value))
							{
								mtab_remove(link->others, sub->key);
								cp_trie_node_delete(grp, link);
							}
							free(attr);
						}
					}
				}
				else if (mtab_count(node->others) == 1) /* case (3) */
				{
					mtab_node *sub = get_single_entry(node->others);
					attr = mergestr(map_node->attr, sub->attr);
					if (attr)
					{
						if (NODE_STORE(link, attr, sub->value))
						{
							mtab_remove(node->others, sub->key);
							cp_trie_node_delete(grp, node);
						}
						free(attr);
					}
				}
			}
			break;
		}
		
		prev = link;
		cprev = ccurr;
		ccurr = *key;
		link = map_node->value; 
	} 

DONE: 
	cp_trie_txunlock(grp);
	return rc;
}

void *cp_trie_exact_match(cp_trie *grp, char *key)
{
	void *last = NULL;
	cp_trie_node *link = grp->root; 
	mtab_node *map_node;
	char *branch = NULL;

	if (cp_trie_txlock(grp, COLLECTION_LOCK_READ)) return NULL; 
 
	while ((map_node = NODE_MATCH(link, key)) != NULL) 
	{
		branch = map_node->attr;
 
		while (*key && *key == *branch)
		{
			key++;
			branch++;
		}
		if (*branch) break; /* mismatch */

		link = map_node->value; 
	} 

	if (link) last = link->leaf;

	cp_trie_txunlock(grp);
 
	if (*key == '\0' && branch && *branch == '\0') 
		return last;  /* prevent match on super-string keys */
	return NULL;
}

/* return a vector containing exact match and any prefix matches */
cp_vector *cp_trie_fetch_matches(cp_trie *grp, char *key)
{
	int rc;
	cp_vector *res = NULL;
	cp_trie_node *link = grp->root;
	mtab_node *map_node;
	char *branch;

	if ((rc = cp_trie_txlock(grp, COLLECTION_LOCK_READ))) return NULL; 
 
	while ((map_node = NODE_MATCH(link, key)) != NULL) 
	{
		branch = map_node->attr;

		while (*key && *key == *branch)
		{
			key++;
			branch++;
		}
		if (*branch) break; /* mismatch */
	
		link = map_node->value; 
		if (link->leaf)
		{ 
			if (res == NULL)
			{
				res = cp_vector_create(1);
				if (res == NULL) break;
			}
			cp_vector_add_element(res, link->leaf);
		} 
	}

	cp_trie_txunlock(grp);
	return res; 
} 

static void extract_subtrie(cp_trie_node *link, cp_vector *v);

static int extract_node(void *n, void *v)
{
	mtab_node *node = n;
	cp_vector *res = v;

	extract_subtrie(node->value, v);

	return 0;
}

static void extract_subtrie(cp_trie_node *link, cp_vector *v)
{
	if (link->leaf)
		cp_vector_add_element(v, link->leaf);

	if (link->others)
		mtab_callback(link->others, extract_node, v);
}

/* return a vector containing all entries in subtree under path given by key */
cp_vector *cp_trie_submatch(cp_trie *grp, char *key)
{
	int rc;
	cp_vector *res = NULL;
	cp_trie_node *link = grp->root;
	mtab_node *map_node;
	char *branch;

	if ((rc = cp_trie_txlock(grp, COLLECTION_LOCK_READ))) return NULL; 
 
	while ((map_node = NODE_MATCH(link, key)) != NULL) 
	{
		branch = map_node->attr;

		while (*key && *key == *branch)
		{
			key++;
			branch++;
		}

		if (*branch && *key) break; /* mismatch */
	
		link = map_node->value; 

		if (*key == '\0')
		{
			res = cp_vector_create(1);
			extract_subtrie(link, res);
			break;
		}
	}

	cp_trie_txunlock(grp);
	return res; 
} 

/* return longest prefix match for given key */
int cp_trie_prefix_match(cp_trie *grp, char *key, void **leaf)
{ 
	int rc, match_count = 0; 
	void *last = grp->root->leaf; 
	cp_trie_node *link = grp->root; 
	mtab_node *map_node;
	char *branch;

	if ((rc = cp_trie_txlock(grp, COLLECTION_LOCK_READ))) return rc; 
 
	while ((map_node = NODE_MATCH(link, key)) != NULL) 
	{ 
		branch = map_node->attr;
 
		while (*key && *key == *branch)
		{
			key++;
			branch++;
		}
		if (*branch) break; /* mismatch */

		link = map_node->value; 
		if (link->leaf)
		{ 
			last = link->leaf; 
			match_count++; 
		} 
	} 

	*leaf = last; 
 
	cp_trie_txunlock(grp);
 
	return match_count; 
} 

int cp_trie_count(cp_trie *grp)
{
	return grp->path_count;
}

void cp_trie_set_root(cp_trie *grp, void *leaf) 
{ 
    grp->root->leaf = leaf; 
} 

/* dump trie to stdout - for debugging */

static int node_count;
static int depth_total;
static int max_level;

void cp_trie_dump_node(cp_trie_node *node, int level, char *prefix)
{
	int i;
	mtab_node *map_node;

	node_count++;
	depth_total += level;
	if (level > max_level) max_level = level;

	for (i = 0; i < node->others->size; i++)
	{
		map_node = node->others->table[i];
		while (map_node)
		{
			cp_trie_dump_node(map_node->value, level + 1, map_node->attr);
			map_node = map_node->next;
		}
	}

	for (i = 0; i < level; i++) printf("\t");

	printf(" - %s => [%s]\n", prefix, node->leaf ? (char *) node->leaf : "");
}
			
void cp_trie_dump(cp_trie *grp)
{
	node_count = 0;
	depth_total = 0;
	max_level = 0;
	
	cp_trie_dump_node(grp->root, 0, "");

#ifdef DEBUG
	printf("\n %d nodes, %d deepest, avg. depth is %.2f\n\n", 
			node_count, max_level, (float) depth_total / node_count);
#endif
}

/* set trie to use given mempool or allocate a new one if pool is NULL */
int cp_trie_use_mempool(cp_trie *trie, cp_mempool *pool)
{
	int rc = 0;
	
	if ((rc = cp_trie_txlock(trie, COLLECTION_LOCK_WRITE))) return rc;
	
	if (pool)
	{
		if (pool->item_size < sizeof(cp_trie_node))
		{
			rc = EINVAL;
			goto DONE;
		}
		if (trie->mempool) 
		{
			if (trie->path_count) 
			{
				rc = ENOTEMPTY;
				goto DONE;
			}
			cp_mempool_destroy(trie->mempool);
		}
		cp_mempool_inc_refcount(pool);
		trie->mempool = pool;
	}
	else
	{
		trie->mempool = 
			cp_mempool_create_by_option(COLLECTION_MODE_NOSYNC, 
										sizeof(cp_trie_node), 0);
		if (trie->mempool == NULL) 
		{
			rc = ENOMEM;
			goto DONE;
		}
	}

DONE:
	cp_trie_txunlock(trie);
	return rc;
}


/* set trie to use a shared memory pool */
int cp_trie_share_mempool(cp_trie *trie, cp_shared_mempool *pool)
{
	int rc;

	if ((rc = cp_trie_txlock(trie, COLLECTION_LOCK_WRITE))) return rc;

	if (trie->mempool)
	{
		if (trie->path_count)
		{
			rc = ENOTEMPTY;
			goto DONE;
		}

		cp_mempool_destroy(trie->mempool);
	}

	trie->mempool = cp_shared_mempool_register(pool, sizeof(cp_trie_node));
	if (trie->mempool == NULL) 
	{
		rc = ENOMEM;
		goto DONE;
	}
	
DONE:
	cp_trie_txunlock(trie);
	return rc;
}


