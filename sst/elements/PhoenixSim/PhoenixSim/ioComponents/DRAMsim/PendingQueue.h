#ifndef PENDINGQUEUE__H
#define PENDINGQUEUE__H

#include "PendingInfo.h"
#include "constants.h"



class PendingQueue {

public:
	PendingQueue(){};


  PendingInfo pq_entry[MAX_TRANSACTION_QUEUE_DEPTH];
  int queue_size; /* Number of valid entries */

	void resort_trans_pending_queue(int updated_entry);
	void del_trans_pending_queue(int tran_id,  int rank_id, int bank_id, int row_id);
	void add_trans_pending_queue( int trans_id, int rank_id, int bank_id, int row_id);
	void update_trans_pending_queue(int transaction_id,int location);
};

#endif

