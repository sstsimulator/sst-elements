#include "RefreshQueue.h"


RefreshQueue::RefreshQueue(){
	rq_head = NULL;
	refresh_rank_index = 0;		/* the rank index to be refreshed - dont care for some policies */
	refresh_bank_index = 0;		/* the bank index to be refreshed - dont care for some policies */
	refresh_row_index = 0;		/* the row index to be refreshed */
	last_refresh_cycle = 0;		/* tells me when last refresh was done */
	refresh_counter = 0;
	refresh_pending = 0;		/* Set to TRUE if we have to refresh and transaction queue was full */
	size = 0;


}

int RefreshQueue::is_refresh_queue_full() {
  if (size >= MAX_REFRESH_QUEUE_DEPTH ) {
	return 1;
  }
  else
	return 0;
}

int RefreshQueue::is_refresh_queue_half_full() {
  if (size >= (MAX_REFRESH_QUEUE_DEPTH >> 1)) {
	return 1;
  }
  else
	return 0;
}

int RefreshQueue::is_refresh_queue_empty() {
  if (size > 0 ) {
	return FALSE;
  }
  else
	return TRUE;
}


