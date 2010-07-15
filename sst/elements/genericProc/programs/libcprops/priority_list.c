
/**
 *  @addtogroup cp_priority_list
 */
/** @{ */
/* @file
 * 
 * priority list implementation. 
 * <p>
 * features
 * <p>
 * <ul>
 * <li> immediate priority queue
 * <li> simple weight based distribution on normal priority queues
 * </ul>
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "collection.h"
#include "common.h"
#include "priority_list.h"

/* fwd declaration for internal lock function */
int cp_priority_list_txlock(cp_priority_list *list, int type);
/* fwd declaration for internal unlock function */
int cp_priority_list_txunlock(cp_priority_list *list);

/**
 * Internal destructor.
 */
static void cp_priority_list_destroy_internal(cp_priority_list *list);

void cp_priority_list_destroy(cp_priority_list *list)
{
    cp_priority_list_destroy_by_option(list, list->mode);
}

void cp_priority_list_destroy_by_option(cp_priority_list *list, int mode)
{
    int i;

    if (list->immediate) cp_list_destroy_by_option(list->immediate, mode);

    if (list->weight) free(list->weight);

    for (i = 0; i < list->normal_priority_queues; i++)
        cp_list_destroy_by_option(list->normal[i], mode);

	if (list->lock)
	{
		cp_lock_destroy(list->lock);
		free(list->lock);
	}

    free(list->normal);
    free(list);
}


cp_priority_list *
	cp_priority_list_create_by_option(int immediate, 
									  int normal_priority_queues,
									  int *weight, 
									  cp_compare_fn compare_fn,
									  cp_copy_fn copy_fn, 
									  cp_destructor_fn item_destructor,
									  int mode)
{
    cp_priority_list *list;
    int i;

    list = (cp_priority_list *) calloc(1, sizeof(cp_priority_list));
    if (list == NULL) 
	{
        errno = ENOMEM;
        return NULL;
    }
    
    list->immediate = NULL;
    list->normal = NULL;
    list->normal_priority_queues = normal_priority_queues;

    if (normal_priority_queues > 0) 
	{
        list->weight = (int *) malloc(normal_priority_queues * sizeof(int));
        if (list->weight == NULL) 
		{
            cp_priority_list_destroy_internal(list);
            errno = ENOMEM;
            return NULL;
        }

        list->normal = (cp_list **) malloc(normal_priority_queues * sizeof(cp_list *));
        if (list->normal == NULL) 
		{
            cp_priority_list_destroy_internal(list);
			errno = ENOMEM;
            return NULL;
        }

        for (i = 0; i < normal_priority_queues; i++) 
		{
            list->weight[i] = weight[i];

            list->normal[i] = cp_list_create_list(mode | COLLECTION_MODE_NOSYNC,
												  compare_fn, copy_fn, 
												  item_destructor);
            if (list->normal[i] == NULL) 
			{
                cp_priority_list_destroy_internal(list);
                errno = ENOMEM;
                return NULL;
            }
        }
    }

    if (immediate) 
	{
        list->immediate = cp_list_create_list(mode | COLLECTION_MODE_NOSYNC,
											  compare_fn, copy_fn,
											  item_destructor);
        if (list->immediate == NULL) 
		{
            cp_priority_list_destroy_internal(list);
            errno = ENOMEM;
            return NULL;
        }
    } 
	else 
        list->immediate = NULL;

	if ((list->lock = malloc(sizeof(cp_lock))) == NULL)
	{
		cp_priority_list_destroy_internal(list);
		errno = ENOMEM;
		return NULL;
	}
    if (cp_lock_init(list->lock, NULL) == -1)
	{
		cp_priority_list_destroy_internal(list);
		return NULL;
	}

    list->mode = mode;
    list->cycle_position = 0;
    list->distribution_counter = 0;
	list->compare_fn = compare_fn;


    return list;    
}

void *cp_priority_list_insert(cp_priority_list *list, void *item, int priority)
{
    return cp_priority_list_insert_by_option(list, item, priority, list->mode);
}

void *cp_priority_list_insert_by_option(cp_priority_list *list, 
										void *item, 
										int priority, 
										int mode)
{
    void *res = NULL;
	int err = 0;

    if (!(mode & COLLECTION_MODE_NOSYNC))
		if (cp_priority_list_txlock(list, COLLECTION_LOCK_WRITE)) return NULL;

	/* if no mutliples allowed, check all inner lists before adding */
	if (!(mode & COLLECTION_MODE_MULTIPLE_VALUES))
	{
		int i;
		
		if (list->compare_fn == NULL)
		{
			err = CP_INVALID_FUNCTION_POINTER;
			goto INSERT_DONE;
		}
		if (list->immediate != NULL && cp_list_search(list->immediate, item))
		{
			err = CP_ITEM_EXISTS;
			goto INSERT_DONE;
		}
		
		for (i = 0; i < list-> normal_priority_queues; i++)
			if (cp_list_search(list->normal[i], item))
			{
				err = CP_ITEM_EXISTS;
				goto INSERT_DONE;
			}
	}
	
    if (priority == 0 && list->immediate != NULL) 
        res = cp_list_insert(list->immediate, item);
    else if (priority > 0 && priority <= list->normal_priority_queues) 
        res = cp_list_insert(list->normal[priority - 1], item);
    else 	// if priority is not supported, add to lowest priority queue
	{
//        res = cp_list_insert(list->normal[list->normal_priority_queues - 1], item);
		res = NULL;
		err = EINVAL;
	}
    

    if (res) list->item_count++; 
	
INSERT_DONE:
    if (!(mode & COLLECTION_MODE_NOSYNC)) cp_priority_list_txunlock(list);

	if (err) errno = err;

    return res;
}

int cp_priority_list_is_empty(cp_priority_list *list)
{
	int rc;
    int empty;

    if ((rc = cp_priority_list_txlock(list, COLLECTION_LOCK_READ))) return rc;
    
    empty = list->item_count == 0;

    cp_priority_list_txunlock(list);

    return empty;
}

void *cp_priority_list_get_next_by_option(cp_priority_list *list, int mode)
{
    void *res = NULL;
    int start_queue;

    if (cp_priority_list_txlock(list, COLLECTION_LOCK_WRITE)) return NULL;;

    if (list->immediate) res = cp_list_remove_tail(list->immediate);
    if (res == NULL && list->normal_priority_queues) 
	{
        if (list->distribution_counter == list->weight[list->cycle_position]) 
		{
            list->distribution_counter = 0;
            list->cycle_position = 
				(list->cycle_position + 1) % list->normal_priority_queues;
        }

        list->distribution_counter++;
        start_queue = list->cycle_position;
        do 
		{
            res = cp_list_remove_tail(list->normal[list->cycle_position]);
            if (res == NULL) 
			{
                list->cycle_position = (list->cycle_position + 1) % list->normal_priority_queues;
                list->distribution_counter = 1;
			}
            else
                break;
        }
        while (list->cycle_position != start_queue);
            
    }

    if (res) list->item_count--;
    cp_priority_list_txunlock(list);

    return res;
}

int cp_priority_list_lock_internal(cp_priority_list *list, int lock_mode)
{
    int rc = -1;

    switch (lock_mode) 
    {
	    case COLLECTION_LOCK_READ:
                rc = cp_lock_rdlock(list->lock);
                break;

	    case COLLECTION_LOCK_WRITE:
                rc = cp_lock_wrlock(list->lock);
                break;

            case COLLECTION_LOCK_NONE:
                rc = 0;
                break;
				
	    default:
                errno = EINVAL;
    }

    return rc;
}

int cp_priority_list_unlock_internal(cp_priority_list *list)
{
	return cp_lock_unlock(list->lock);
}

int cp_priority_list_txlock(cp_priority_list *list, int type)
{
	if (list->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (list->mode & COLLECTION_MODE_IN_TRANSACTION && 
		list->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, list->txowner)) return 0;
	}
	return cp_priority_list_lock_internal(list, type);
}

int cp_priority_list_txunlock(cp_priority_list *list)
{
	if (list->mode & COLLECTION_MODE_NOSYNC) return 0;
	if (list->mode & COLLECTION_MODE_IN_TRANSACTION && 
		list->txtype == COLLECTION_LOCK_WRITE)
	{
		cp_thread self = cp_thread_self();
		if (cp_thread_equal(self, list->txowner)) return 0;
	}
	return cp_priority_list_unlock_internal(list);
}

/* lock and set the transaction indicators */
int cp_priority_list_lock(cp_priority_list *list, int type)
{
	int rc;
	if ((list->mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	if ((rc = cp_priority_list_lock_internal(list, type))) return rc;
	list->txtype = type;
	list->txowner = cp_thread_self();
	list->mode |= COLLECTION_MODE_IN_TRANSACTION;
	return 0;
}

/* unset the transaction indicators and unlock */
int cp_priority_list_unlock(cp_priority_list *list)
{
	cp_thread self = cp_thread_self();
	if (list->txowner == self)
	{
		list->txtype = 0;
		list->txowner = 0;
		list->mode ^= COLLECTION_MODE_IN_TRANSACTION;
	}
	else if (list->txtype == COLLECTION_LOCK_WRITE)
		return -1;
	return cp_priority_list_unlock_internal(list);
}

/* get the current collection mode */
int cp_priority_list_get_mode(cp_priority_list *list)
{
    return list->mode;
}

/* set mode bits on the list mode indicator */
int cp_priority_list_set_mode(cp_priority_list *list, int mode)
{
	int nosync;

	/* can't set NOSYNC in the middle of a transaction */
	if ((list->mode & COLLECTION_MODE_IN_TRANSACTION) && 
		(mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
	
	nosync = list->mode & COLLECTION_MODE_NOSYNC;
	if (!nosync)
		if (cp_priority_list_txlock(list, COLLECTION_LOCK_WRITE))
			return -1;

	list->mode |= mode;

	if (!nosync)
		cp_priority_list_txunlock(list);

	return 0;
}

/* unset mode bits on the list mode indicator. if unsetting 
 * COLLECTION_MODE_NOSYNC and the list was not previously synchronized, the 
 * internal synchronization structure is initalized.
 */
int cp_priority_list_unset_mode(cp_priority_list *list, int mode)
{
	int nosync = list->mode & COLLECTION_MODE_NOSYNC;

	if (!nosync)
		if (cp_priority_list_txlock(list, COLLECTION_LOCK_WRITE))
			return -1;
	
	/* handle the special case of unsetting COLLECTION_MODE_NOSYNC */
	if ((mode & COLLECTION_MODE_NOSYNC) && list->lock == NULL)
	{
		/* list can't be locked in this case, no need to unlock on failure */
		if ((list->lock = malloc(sizeof(cp_lock))) == NULL)
			return -1; 
		if (cp_lock_init(list->lock, NULL))
			return -1;
	}
	
	/* unset specified bits */
    list->mode &= list->mode ^ mode;
	if (!nosync)
		cp_priority_list_txunlock(list);

	return 0;
}


long cp_priority_list_item_count(cp_priority_list *list)
{
    return list->item_count;
}

/** @} */

static void cp_priority_list_destroy_internal(cp_priority_list *list)
{
    int i;

    if (list->weight) free(list->weight);
    if (list->immediate) cp_list_destroy(list->immediate);
    if (list->normal) 
    {
        for (i = 0; i < list->normal_priority_queues; i++)
        {
            if (list->normal[i] == NULL) break;
            cp_list_destroy(list->normal[i]);
        }
        free(list->normal);
    }
	if (list->lock) 
	{
		cp_lock_destroy(list->lock);
		free(list->lock);
	}
    free(list);
}

