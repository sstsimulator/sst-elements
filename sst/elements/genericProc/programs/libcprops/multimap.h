#ifndef _CP_MULTIMAP_H
#define _CP_MULTIMAP_H

/** @{ */
/**
 * @file
 *
 * cp_multimap is a data structure designed to support multiple indices. 
 * Indices may be defined at creation time or added dynamically after creation.
 * The map supports access by key, by index or by CP_OP_* operator to match
 * the closest entry to the requested entry. 
 *
 * cp_multimap is somewhat different to other mapping collections in that
 * there is no mapping as such. Client code supplies entries, a comparison
 * function and optionaly a key function to extract a key from an entry. If
 * no key function is given entries are passed directly to the comparison 
 * function for internal lookups. 
 *
 * The table is instantiated with a primary index. Additional indices may be 
 * added subsequently. Adding an index implies a full tree scan to create an
 * additional view on the tree sorted by the new index. cp_multimap_get() 
 * performs a lookup using the primary index, cp_multimap_get_by_index() 
 * performs a lookup using a secondary index. As a special case, 
 * cp_multimap_get_by_index() performs a lookup by the primary index if given
 * a NULL index descriptor. 
 *
 * Example:
 *
 * struct subscriber
 * {
 *     char first_name[32];
 *     char last_name[32];
 *     char phone_num[32];
 * };
 *
 * int subscriber_name_compare(void *a, void *b)
 * {
 *     int cmp;
 *     struct subscriber *s1 = (struct subscriber *) a;
 *     struct subscriber *s2 = (struct subscriber *) b;
 *
 *     cmp = strcmp(s1->last_name, s2->last_name);
 *     if (cmp == 0)
 *         cmp = strcmp(s1->first_name, s2->first_name);
 *
 *     return cmp;
 * }
 *
 * int subscriber_number_compare(void *a, void *b)
 * {
 *     int n1, n2;
 *
 *     n1 = atoi((char *) s1);
 *     n2 = atoi((char *) s2);
 *
 *     return n1 - n2;
 * }
 *
 * int subscriber_number_key(void *entry)
 * {
 *     struct subscriber *s = (struct subscriber *) entry;
 *     return &s->phone_num;
 * }
 *
 * ...
 *
 * int err;
 * cp_index *index;
 * char number[32];
 * struct subscriber x, *res;
 *
 * struct subscriber *a = create_subscriber("Duck", "Daffy", "123");
 * struct subscriber *b = create_subscriber("Fudge", "Elmore", "456");
 * struct subscriber *c = create_subscriber("Fudge", "Elvis", "789");
 * 
 * cp_multimap *subscriber_list = 
 *     cp_multimap_create(NULL, subscriber_name_compare);
 *
 * index = cp_multimap_create_index(subscriber_list, 
 *                                  CP_UNIQUE, 
 *                                  subscriber_number_key, 
 *                                  subscriber_number_compare, 
 *                                  &err);
 * if (index == NULL) 
 *    cp_error(err, "can\'t create index"); // ... handle error
 *
 * // find Daffy's number
 *
 * sprintf(x.first_name, "Daffy");
 * sprintf(x.last_name, "Duck");
 *
 * res = cp_multi_map_get(subscriber_list, &x);
 * if (res)
 *     printf("Daffy Duck: %s\n", res->phone_number);
 * else
 *     printf("No entry for Daffy Duck\n");
 *
 * // reverse lookup phone number 789
 *
 * sprintf(number, "789");
 * res = cp_multimap_get_by_index(subscriber_list, index, number);
 * if (res)
 *     printf("number 789 belongs to %s %s\n", res->first_name, res->last_name);
 * else
 *     printf("no subscriber with number 789\n");
 *
 * ...
 *
 */

#include "common.h"

__BEGIN_DECLS

#include "rb.h"
#include "hashlist.h"
#include "vector.h"
#include "mempool.h"

struct _cp_multimap;

#define MM_RED    0
#define MM_BLACK  1

typedef CPROPS_DLL struct _cp_index_map_node
{
    void *entry;

    /* balance maintainance - color is either 'red' or 'black' */
    int color;

    struct _cp_index_map_node *left;
    struct _cp_index_map_node *right;
    struct _cp_index_map_node *up;
} cp_index_map_node;

/* (internal) allocate a new node */
CPROPS_DLL
cp_index_map_node *
    cp_index_map_node_create(void *entry, struct _cp_mempool *pool);
/* (internal) deallocate a node */
CPROPS_DLL
void cp_multimap_destroy_node(struct _cp_multimap *owner, 
                              cp_index_map_node *node);
/* (internal) deallocate a node and its subnodes */
CPROPS_DLL
void cp_multimap_destroy_node_deep(struct _cp_multimap *owner, 
                                   cp_index_map_node *node);

struct _cp_multimap;

typedef CPROPS_DLL struct _cp_index_map
{
    struct _cp_multimap *owner;  /* owning multimap */

    cp_index_map_node *root;     /* root node */

    int mode;                    /* mode flags */
    cp_index *index;             /* index information */
} cp_index_map;

cp_index_map *
    cp_index_map_create(struct _cp_multimap *owner, int mode, cp_index *index);

/* (internal) reposition an entry */
CPROPS_DLL
int cp_index_map_reindex(cp_index_map *tree, void *before, void *after);


/* tree wrapper object */
typedef CPROPS_DLL struct _cp_multimap
{
    int mode;                     /* mode flags */

    int items;                    /* item count */
    
    cp_copy_fn copy;              /* entry copy function */
    cp_destructor_fn dtr;         /* entry destructor */

    cp_index default_index;       /* default index */
    cp_index_map *default_map;    /* default map */

	cp_index_map *primary;		  /* primary unique index - identical to 
								   * default if default is unique */

    cp_hashlist *index;           /* index table */

    cp_lock *lock;
    cp_thread txowner;            /* set if a transaction is in progress */
    int txtype;                   /* lock type */

    cp_mempool *mempool;          /* optional memory pool */
} cp_multimap;

/* 
 * default create function - equivalent to create_by_option with mode 
 * COLLECTION_MODE_NOSYNC
 */
CPROPS_DLL
cp_multimap *cp_multimap_create(cp_key_fn key_fn, cp_compare_fn cmp);

/*
 * create function for multiple value trees, allows specifying mapping 
 * comparison function to delete individual mappings. The mode parameter
 * is logically or'ed with COLLECTION_MODE_MULTIPLE_VALUES.
 */
CPROPS_DLL
cp_multimap *
    cp_multimap_create_by_option(int mode, cp_key_fn key_fn, 
	                             cp_compare_fn cmp, 
                                 cp_copy_fn copy, 
                                 cp_destructor_fn dtr);

/* 
 * recursively destroy the tree structure 
 */
CPROPS_DLL
void cp_multimap_destroy(cp_multimap *tree);
/*
 * recursively destroy the tree structure with the given destructor functions
 */
CPROPS_DLL
void cp_multimap_destroy_custom(cp_multimap *tree, cp_destructor_fn dtr);

/*
 * check whether this entry is already present on any unique index. returns
 * non-zero if an matching entry on a non-unique index was found. If index
 * parameter is non-null, it is set to the first index an entry was found on. 
 * If existing parameter is non-null, it is set to the first entry found.
 */
CPROPS_DLL
int cp_multimap_get_unique(cp_multimap *tree, 
						   void *entry, 
						   cp_index **index, 
						   void **existing);

/* insertion function */
CPROPS_DLL
void *cp_multimap_insert(cp_multimap *tree, void *entry, int *err);

/* retrieve the entry described by the given key */
CPROPS_DLL
void *cp_multimap_get(cp_multimap *tree, void *entry);

/* retrieve the entry described by the given key using the given index */
CPROPS_DLL
void *cp_multimap_get_by_index(cp_multimap *map, cp_index *index, void *entry);

/* find the value of the closest key by operator */
CPROPS_DLL
void *cp_multimap_find(cp_multimap *map, void *key, cp_op op);

/* find the value of the closest key by operator and by the given index */
CPROPS_DLL
void *cp_multimap_find_by_index(cp_multimap *map, 
								cp_index *index, 
								void *entry, 
								cp_op op);

/* return non-zero if entry could be found on any index */
CPROPS_DLL
int cp_multimap_contains(cp_multimap *tree, void *entry);

/* remove a mapping */
CPROPS_DLL
void *cp_multimap_remove(cp_multimap *tree, void *entry);

/* delete mapping(s) matching given entry by given index */
CPROPS_DLL
void *cp_multimap_remove_by_index(cp_multimap *map, 
                                  cp_index *index, 
                                  void *entry);

/* scan indices and replace entries */
CPROPS_DLL
int cp_multimap_reindex(cp_multimap *map, void *before, void *after);

/* 
 * perform a pre-order iteration over the tree, calling 'callback' on each 
 * node
 */
CPROPS_DLL
int cp_multimap_callback_preorder(cp_multimap *tree, 
                                cp_callback_fn callback, 
                                void *prm);
/* 
 * perform an in-order iteration over the tree, calling 'callback' on each 
 * node
 */
CPROPS_DLL
int cp_multimap_callback(cp_multimap *tree, cp_callback_fn callback, void *prm);

/* 
 * perform an in-order iteration over the tree, calling 'callback' on each node
 */
CPROPS_DLL
int cp_multimap_callback_by_index(cp_multimap *tree, 
                                  cp_index *index, 
								  cp_callback_fn callback, 
								  void *prm);

/* 
 * perform a post-order iteration over the tree, calling 'callback' on each 
 * node
 */
CPROPS_DLL
int cp_multimap_callback_postorder(cp_multimap *tree, 
                                   cp_callback_fn callback, 
                                   void *prm);

/* return the number of mappings in the tree */
CPROPS_DLL
int cp_multimap_count(cp_multimap *tree);

#define cp_multimap_size cp_multimap_count

/* 
 * lock tree for reading or writing as specified by type parameter. 
 */
CPROPS_DLL
int cp_multimap_lock(cp_multimap *tree, int type);
/* read lock */
#define cp_multimap_rdlock(tree) (cp_multimap_lock((tree), COLLECTION_LOCK_READ))
/* write lock */
#define cp_multimap_wrlock(tree) (cp_multimap_lock((tree), COLLECTION_LOCK_WRITE))
/* unlock */
CPROPS_DLL
int cp_multimap_unlock(cp_multimap *tree);


/* return the table mode indicator */
CPROPS_DLL
int cp_multimap_get_mode(cp_multimap *tree);
/* set mode bits on the tree mode indicator */
CPROPS_DLL
int cp_multimap_set_mode(cp_multimap *tree, int mode);
/* unset mode bits on the tree mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the tree was not previously synchronized, the 
 * internal synchronization structure is initalized.
 */
CPROPS_DLL
int cp_multimap_unset_mode(cp_multimap *tree, int mode);


/** print tree to stdout */
CPROPS_DLL
void cp_multimap_dump(cp_multimap *tree);

/* set tree to use given mempool or allocate a new one if pool is NULL */
CPROPS_DLL
int cp_multimap_use_mempool(cp_multimap *tree, cp_mempool *pool);

/* set tree to use a shared memory pool */
CPROPS_DLL
int cp_multimap_share_mempool(cp_multimap *tree, 
                              struct _cp_shared_mempool *pool);

/* add an index */
CPROPS_DLL
cp_index *cp_multimap_create_index(cp_multimap *map, 
                                   cp_index_type type,
                                   cp_key_fn key, 
                                   cp_compare_fn cmp,
                                   int *err);

__END_DECLS

/** @} */

#endif

