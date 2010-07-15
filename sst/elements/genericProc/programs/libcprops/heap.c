/* cp_heap is a generic heap implementation based on libheap and contributed 
 * to cprops by Kyle Wheeler. The original copyright notice follows.
 *
 * This file (a generic C heap implementation) is licensed under the BSD
 * license, and is copyrighted to Kyle Wheeler and Branden Moore, 2003 and
 * 2004. Feel free to distribute and use as you see fit, as long as this
 * copyright notice stays with the code.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "heap.h"

#define PARENT(x) ((x) >> 1)
#define LEVELSIZE(x) (1 << (x))
#define LEFTCHILD(x) ((x<<1))

/* functions */
static int  heap_resize(cp_heap *);
static void siftup(cp_heap *h);
static void siftdown(cp_heap *, unsigned int);
static void swap(void **, void **);

unsigned int cp_heap_count(cp_heap *h)
{
    return h->heap_end - 1;
}

unsigned int cp_heap_size(cp_heap *h)
{
    return h->heap_length;
}

cp_heap *cp_heap_create_by_option(int mode, 
                                  int initial_size,
                                  cp_compare_fn comp,
                                  cp_copy_fn copy,
                                  cp_destructor_fn dtr)
{
    int rc;
    cp_heap *h = (cp_heap *) calloc(1, sizeof(cp_heap));
    if (h == NULL) return NULL;

    if ((mode & COLLECTION_MODE_NOSYNC) == 0)
    {
        h->lock = malloc(sizeof(cp_mutex));
        if (h->lock == NULL)
        {
            free(h);
            return NULL;
        }
        if ((rc = cp_mutex_init(h->lock, NULL)))
        {
            free(h->lock);
            free(h);
            return NULL;
        }
    }

    if (initial_size)
    {
        for (h->heap_length = 0; 
             h->heap_length < initial_size; 
             h->heap_height++)
            h->heap_length += LEVELSIZE(h->heap_height) + 1;
        if ((h->heap = malloc(h->heap_length * sizeof(void *))) == NULL)
        {
            cp_heap_destroy(h);
            return NULL;
        }
    }
    else
    {
        h->heap = NULL;
        h->heap_height = 0;
        h->heap_length = 0;
    }

    h->heap_end = 1;
    h->comp = comp;
    h->copy = copy;
    h->dtr = dtr;
    h->mode = mode;

#ifdef __TRACE__
    DEBUGMSG("cp_heap_create_by_option\n");
    heap_info(h);
#endif
    return h;
}

cp_heap *cp_heap_create(cp_compare_fn cmp)
{
    return cp_heap_create_by_option(0, 0, cmp, NULL, NULL);
}

void cp_heap_destroy(cp_heap *h)
{
#ifdef __TRACE__
    DEBUGMSG("cp_heap_destroy\n");
    heap_info(h);
#endif
    if (h) 
    {
        if (h->heap) 
        {
            if ((h->mode & COLLECTION_MODE_DEEP) && h->dtr)
            {
                int i;
                for (i = 1; i < h->heap_end; ++i) 
                    if (h->heap[i]) (*h->dtr)(h->heap[i]);
            }
            free(h->heap);
        }

        if (h->lock)
        {
            cp_mutex_destroy(h->lock);
            free(h->lock);
        }

        free(h);
    }
}

int cp_heap_push(cp_heap *h, void *in)
{
    int rc = 0;
    unsigned int he;

    if ((rc = cp_heap_txlock(h))) return rc;

    he = h->heap_end;

#ifdef _HEAP_TRACE
    DEBUGMSG("cp_heap_push: %p\n", in);
    heap_info(h);
#endif
    if ((rc = heap_resize(h))) goto DONE;
    if ((h->mode & COLLECTION_MODE_COPY) && h->copy)
        h->heap[he] = (*h->copy)(in);
    else
        h->heap[he] = in;
    siftup(h);
#ifdef _HEAP_TRACE
    DEBUGMSG("cp_heap_push end\n");
    heap_info(h);
#endif
DONE:
    cp_heap_txunlock(h);
    return rc;
}

void *cp_heap_peek(cp_heap *h)
{
    void *item = NULL;
    if (h == NULL) return NULL;
    errno = 0;
    if (cp_heap_txlock(h)) return NULL;
    if (h->heap && h->heap_end > 1) item = (void *)(h->heap[1]);
    cp_heap_txunlock(h);
    return item;
}

void *cp_heap_pop(cp_heap *h)
{
    int rc;
    void *retval = NULL;

    if (h == NULL) return NULL;

    if ((rc = cp_heap_txlock(h))) return NULL;

    if (h->heap != NULL && h->heap_end > 1) 
    {
        retval = h->heap[1];
#ifdef _HEAP_TRACE
        DEBUGMSG("cp_heap_pop: %p\n", retval);
#endif
        if ((h->mode & COLLECTION_MODE_DEEP) && h->dtr)
            (*h->dtr)(retval);

        h->heap[1] = h->heap[h->heap_end - 1];
        h->heap_end--;
        siftdown(h, 1);
    }

    cp_heap_txunlock(h);

    return retval;
}

void cp_heap_callback(cp_heap *h, cp_callback_fn cb, void *prm)
{
    int rc;
    unsigned int i;

    if (h == NULL) return;

    if ((rc = cp_heap_txlock(h))) return;

    if (!h->heap || h->heap_end <= 1) goto DONE;

    for (i = 1; i < h->heap_end; i++) 
        (*cb)(h->heap[i], prm);

DONE:
    cp_heap_txunlock(h);
}

static void siftdown(cp_heap *h, unsigned int p)
{
    unsigned int c;

#ifdef _HEAP_TRACE
    DEBUGMSG("siftdown start\n");
    heap_info(h);
    fflush(stdout);
#endif
    for (c = LEFTCHILD(p); c < h->heap_end; p = c, c = LEFTCHILD(p)) 
    {
#ifdef _HEAP_TRACE
        DEBUGMSG(" c = %i, h->heap_end = %i\n", c, h->heap_end);
#endif
        if (h->comp(h->heap[c], h->heap[c + 1]) <= 0) 
            c++;
        
        /* c points to the largest among the children of p */
        if (h->comp(h->heap[p], h->heap[c]) <= 0) 
            swap(&(h->heap[p]), &(h->heap[c]));
        else 
            return;
    }
    if (c == h->heap_end && h->comp(h->heap[p], h->heap[c]) <= 0) 
        swap(&(h->heap[p]), &(h->heap[c]));
}

static int heap_resize(cp_heap *h)
{
    if (h->heap_end + 1 >= h->heap_length) 
    {
        /* resize heap */
        void **temp;
        unsigned int new_length =
            h->heap_length + LEVELSIZE(h->heap_height) + 1;

#ifdef __TRACE__
        DEBUGMSG("heap [%p] resize %d -> %d", h, h->heap_length, new_length);
#endif 

        temp = realloc(h->heap, new_length * sizeof(void *));
        if (temp == NULL) 
        {
            cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t resize heap");
            return -1;
        }

        h->heap = temp;
        h->heap_height++;
        h->heap_length = new_length;
        h->heap_end++;
    } 
    else /* no resize */
        h->heap_end++;

    return 0;
}

int cp_heap_contract(cp_heap *h)
{
    int rc;
    unsigned int new_length;
    unsigned int tmp_length;
    unsigned int new_height;

    if ((rc = cp_heap_txlock(h))) return rc;

    tmp_length = h->heap_length;
    new_height = h->heap_height;

    while (1)
    {
        new_length = tmp_length;
        tmp_length -= LEVELSIZE(new_height - 1) + 1;
        if (tmp_length < h->heap_end) break;
        new_height--;
    }

    if (new_length != h->heap_length)
    {
        void **new_heap;
#ifdef __TRACE__
        DEBUGMSG("heap [%p] contracting %d to %d", 
                 h, h->heap_length, new_length);
#endif
        new_heap = realloc(h->heap, new_length * sizeof(void *));
        if (new_heap == NULL) 
        {
            rc = -1;
            goto DONE;
        }
        h->heap = new_heap;
        h->heap_length = new_length;
        h->heap_height = new_height;
    }

DONE:
    cp_heap_txunlock(h);
    return rc;
}

static void swap(void **a, void **b)
{
    void *c = *a;

    *a = *b;
    *b = c;
}

static void siftup(cp_heap *h)
{
    unsigned int c = h->heap_end - 1;

    while (c != 1 && h->comp(h->heap[PARENT(c)], h->heap[c]) <= 0) 
    {
#ifdef _HEAP_TRACE
        DEBUGMSG("siftup\n");
        heap_info(h);
        fflush(stdout);
#endif
        swap(&(h->heap[PARENT(c)]), &(h->heap[c]));
        c = PARENT(c);
    }
}

#if defined(DEBUG) || defined(_HEAP_TRACE) || defined (__TRACE__)
void heap_info(cp_heap *h)
{
    DEBUGMSG(" heap->heap: %p\n", h->heap);
    DEBUGMSG("     ->heap_height: %i\n", h->heap_height);
    DEBUGMSG("     ->heap_end:    %i\n", h->heap_end);
    DEBUGMSG("     ->heap_length: %i\n", h->heap_length);
    DEBUGMSG("     ->comp:       %p\n", h->comp);
}
#endif

#ifdef DEBUG
int cp_heap_verify(cp_heap *h)
{
    unsigned int i;

    if (!h || !h->heap || h->heap_end <= 1) return 1;

    for (i = h->heap_end - 1; i > PARENT(h->heap_end - 1); --i) 
    {
        unsigned int c = i;

        while (c != 1 && !(h->comp(h->heap[PARENT(c)], h->heap[c]))) 
            c = PARENT(c);

        if (c != 1) return 0;
    }
    return 1;
}
#endif

int cp_heap_txlock(cp_heap *heap)
{
    if (heap->mode & COLLECTION_MODE_NOSYNC) return 0;
    if (heap->mode & COLLECTION_MODE_IN_TRANSACTION)
    {
        cp_thread self = cp_thread_self();
        if (cp_thread_equal(self, heap->txowner)) return 0;
    }
    errno = 0;
    return cp_mutex_lock(heap->lock);
}

int cp_heap_txunlock(cp_heap *heap)
{
    if (heap->mode & COLLECTION_MODE_NOSYNC) return 0;
    if (heap->mode & COLLECTION_MODE_IN_TRANSACTION)
    {
        cp_thread self = cp_thread_self();
        if (cp_thread_equal(self, heap->txowner)) return 0;
    }
    return cp_mutex_unlock(heap->lock);
}

int cp_heap_lock(cp_heap *heap)
{
    int rc;
    if ((heap->mode & COLLECTION_MODE_NOSYNC)) return EINVAL;
    if ((rc = cp_mutex_lock(heap->lock))) return rc;
    heap->txowner = cp_thread_self();
    heap->mode |= COLLECTION_MODE_IN_TRANSACTION;
    return 0;
}

int cp_heap_unlock(cp_heap *heap)
{
    cp_thread self = cp_thread_self();
    if (heap->txowner == self)
    {
        heap->txowner = 0;
        heap->mode ^= COLLECTION_MODE_IN_TRANSACTION;
    }
    return cp_mutex_unlock(heap->lock);
}

int cp_heap_get_mode(cp_heap *heap)
{
    return heap->mode;
}

/* set mode bits */
int cp_heap_set_mode(cp_heap *heap, int mode)
{
    int rc;
    int nosync;

    /* can't set nosync in the middle of a transaction */
    if ((heap->mode & COLLECTION_MODE_IN_TRANSACTION) && 
        (mode & COLLECTION_MODE_NOSYNC)) return EINVAL;

    if ((rc = cp_heap_txlock(heap))) return rc;

    nosync = heap->mode & COLLECTION_MODE_NOSYNC;

    heap->mode |= mode;

    if (!nosync)
        cp_heap_txunlock(heap);

    return 0;
}

int cp_heap_unset_mode(cp_heap *heap, int mode)
{
    int rc;
    int nosync;

    if ((rc = cp_heap_txlock(heap))) return rc;

    nosync = heap->mode & COLLECTION_MODE_NOSYNC;
    
    /* handle the special case of unsetting NOSYNC */
    if ((mode & COLLECTION_MODE_NOSYNC) && heap->lock == NULL)
    {
        if ((heap->lock = malloc(sizeof(cp_mutex))) == NULL)
            return -1;
        if ((rc = cp_mutex_init(heap->lock, NULL)))
        {
            free(heap->lock);
            heap->lock = NULL;
            return -1;
        }
    }

    heap->mode &= heap->mode ^ mode;
    if (!nosync)
        cp_heap_txunlock(heap);

    return 0;
}

