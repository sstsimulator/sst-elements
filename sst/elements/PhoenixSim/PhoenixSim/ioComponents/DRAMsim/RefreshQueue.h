#ifndef REFRESHQUEUE__H
#define REFRESHQUEUE__H

#include "Command.h"
#include <stdlib.h>

class RefreshQueue {

public:
	RefreshQueue();


	Command		*rq_head;				/* Pointer to head of refresh command queue */
	int			refresh_rank_index;		/* the rank index to be refreshed - dont care for some policies */
	int			refresh_bank_index;		/* the bank index to be refreshed - dont care for some policies */
	int			refresh_row_index;		/* the row index to be refreshed */
	tick_t		last_refresh_cycle;		/* tells me when last refresh was done */
	uint64_t refresh_counter;
	int 		refresh_pending;		/* Set to TRUE if we have to refresh and transaction queue was full */
	int 		size; 					/* Tells you how many refresh commands are currently active */



	int is_refresh_queue_full();

	int is_refresh_queue_half_full();
	int is_refresh_queue_empty();

};

#endif




