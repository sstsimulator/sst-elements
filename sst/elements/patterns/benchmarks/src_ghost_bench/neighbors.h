/*
** $Id: neighbors.h,v 1.1 2010-10-22 22:14:54 rolf Exp $
**
** Rolf Riesen, Sandia National Laboratories, July 2010
** Determine the neighbor rank IDs
*/

#ifndef _NEIGHBORS_H_
#define _NEIGHBORS_H_

/* Who are my neighbors */
typedef struct neighbors_t   {
    int right;
    int left;
    int up;
    int down;
    int back;
    int front;
} neighbors_t;

void neighbors(int verbose, int me, int width, int height, int depth, neighbors_t *n);

#endif /* _NEIGHBORS_H_ */
