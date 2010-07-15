#ifndef _CP_PRIORITY_LIST_H
#define _CP_PRIORITY_LIST_H

/**
 * @addtogroup cp_priority_list
 */
/** @{ */

#include "common.h"

__BEGIN_DECLS

#include "config.h"

#include "collection.h"
#include "linked_list.h"

#define PRIORITY_LIST_IMMEDIATE 1
#define PRIORITY_LIST_NORMAL    2

/**
 * @file
 * push your data on a cp_priority_list then retrieve them by priority. priority 
 * levels are 
 * 0 - immediate: a priority list instantiated with an immediate priority will 
 *     only return immediate priority items as long as such items are present
 *     on the immediate priority sub queue.
 * 1 - the first 'normal priority' subqueue defined. Subqueues are defined with 
 *     a weight. If you define the first subqueue with say weight 2 and the
 *     second with 1, in the absence of an immediate priority subqueue,
 *     calling cp_priority_list_get_next will return 2 items from the queue with
 *     weight 2, then 1 item from the queue with weight 1. Do not confuse the
 *     subqueue index, which is used to determine which queue you push items
 *     on (ie, what 'priority' they receive) with the actual weights. You
 *     could easily construct a cp_priority_list where priority 1 items have a
 *     lower weight than priority 2 items and vice versa. 
 * 
 */

typedef CPROPS_DLL struct _cp_priority_list
{
    cp_list *immediate;         /**< high priority queue for critical things */
    cp_list **normal;           /**< normal priority queues with precedence 
								     weights */
    int *weight;                /**< table of weights for the normal queues */
    int normal_priority_queues; /**< number of normal priorities (supported 
								     priority levels) */
    int distribution_counter;   /**<  */
    int cycle_position;         /**<  */
    int mode;                   /**< locking/operation mode */
	cp_thread txowner; 			/**< transaction lock owner */
	cp_compare_fn compare_fn;

    int item_count;             /**< number of elements in all queues */

    cp_lock *lock;				/**< locking object */
	int txtype;                 /**< lock type */
} cp_priority_list;


/**
 * Internal destructor.
 */
//void cp_priority_list_destroy_internal(cp_priority_list *list);

/**
 * Simplified constructor.
 *
 * @param immediate     if true, create an immediate queue
 * @param normal_priority_queues
 * @param weights       table of weight factors
 */
#define cp_priority_list_create(immediate, normal_priorities, weights) \
    cp_priority_list_create_by_option(immediate, normal_priorities, weights, \
						NULL, NULL, NULL, COLLECTION_MODE_MULTIPLE_VALUES)

/**
 * Constructor with all parameters.
 *
 * @param immediate     	if true, create an immediate queue
 * @param normal_priorities	number of subqueues
 * @param weights       	table of weight factors
 * @param compare_fn       	compare method
 * @param copy_fn       	copy method
 * @param mode          	operation and locking mode
 */
CPROPS_DLL
cp_priority_list *
	cp_priority_list_create_by_option(int immediate, 
									  int normal_priority_queues, 
									  int *weights, 
									  cp_compare_fn compare_fn, 
									  cp_copy_fn copy_fn, 
									  cp_destructor_fn item_destructor,
									  int mode);

/**
 * Destructor
 */
CPROPS_DLL
void cp_priority_list_destroy(cp_priority_list *list);
//#define cp_priority_list_destroy(list) cp_priority_list_destroy_by_option((list), (list)->mode)

/**
 * Destructor with locking option.
 */
CPROPS_DLL
void cp_priority_list_destroy_by_option(cp_priority_list *list, int option);

/**
 * Inserts an entry to the list with priority and default-mode.
 *
 * The entry is added to the list which holds the entries with given priority.
 * @param list the object
 * @param item the entry object
 * @param priority priority of the entry
 *
 * @note If the priority is out of supported range, the items are added to
 *       the queue with lowest priority .
 */
CPROPS_DLL
void *cp_priority_list_insert(cp_priority_list *list, void *item, int priority);

/**
 * Inserts an entry to the list with priority and mode.
 *
 * The entry is added to the list which holds the entries with given priority.
 * @param list the object
 * @param item the entry object
 * @param priority priority of the entry
 * @param mode locking mode
 *
 * @note If the priority is out of supported range, the items are added to
 *       the queue with lowest priority .
 */
CPROPS_DLL
void *cp_priority_list_insert_by_option(cp_priority_list *list, void *item, int priority, int mode);

/**
 * Get the "first" entry of the list with default-mode.
 *
 * The internal algorithm selects one of the non-empty list and 
 * returns the first entry of that list.
 */
//void *cp_priority_list_get_next(cp_priority_list *list);
#define cp_priority_list_get_next(l) cp_priority_list_get_next_by_option((l), (l)->mode)

/**
 * Get the "first" entry of the list with mode.
 *
 * The internal algorithm selects one of the non-empty list and 
 * returns the first entry of that list.
 */
CPROPS_DLL
void *cp_priority_list_get_next_by_option(cp_priority_list *list, int mode);

/**
 * Test if object is empty.
 *
 * @retval true if no element contained.
 * @retval false if at least one element is contained.
 */
CPROPS_DLL
int cp_priority_list_is_empty(cp_priority_list *list);


/**
 * Lock the collection for reading.
 */
#define cp_priority_list_rdlock(list) cp_priority_list_lock(list, COLLECTION_LOCK_READ)

/**
 * Lock the collection for writing.
 */
#define cp_priority_list_wrlock(list) cp_priority_list_lock(list, COLLECTION_LOCK_WRITE)

/**
 * Lock the collection with mode.
 *
 * @param list the object
 * @param lock_mode locking mode
 */
CPROPS_DLL
int cp_priority_list_lock(cp_priority_list *list, int lock_mode);

/**
 * Unlock the collection.
 */
CPROPS_DLL
int cp_priority_list_unlock(cp_priority_list *list);

/**
 * Number of entries in the whole collection.
 *
 * @return number of entries.
 */
CPROPS_DLL
long cp_priority_list_item_count(cp_priority_list *list);

__END_DECLS

/** @} */
#endif 

