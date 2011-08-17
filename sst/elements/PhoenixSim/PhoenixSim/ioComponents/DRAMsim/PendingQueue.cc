#include "PendingQueue.h"




void PendingQueue::del_trans_pending_queue(int tran_id, int rank_id, int bank_id, int row_id){

	
  /* Walk through pending queue -> if you get a match -> update and sort queue 
   * Else add at the end 
   */
  int i;
  int tran_count;
  for (i=0;i<queue_size;i++) {
	if (pq_entry[i].rank_id == rank_id && 
		pq_entry[i].bank_id == bank_id &&
		pq_entry[i].row_id == row_id) {
	    int j,k;
		for (j=0;j<pq_entry[i].transaction_count && pq_entry[i].transaction_list[j] != tran_id;j++);
		for (k=j;k<pq_entry[i].transaction_count-1;k++) 
		  pq_entry[i].transaction_list[k] = pq_entry[i].transaction_list[k+1];

	  	pq_entry[i].transaction_count--;
		// Needed to keep track of whether queue size needs to be reduced
		tran_count = 	pq_entry[i].transaction_count;
		
		resort_trans_pending_queue(i);
		// Reduce queue size only after resorting of queue to prevent loss of
		// last elements
		if (tran_count == 0) 
		  queue_size--;
		break;
	}
  }


}

void PendingQueue::add_trans_pending_queue(int trans_id, int rank_id, int bank_id, int row_id){

	
  /* Walk through pending queue -> if you get a match -> update and sort queue 
   * Else add at the end 
   */
  int i;

  for (i=0;i<queue_size;i++) {
	if (pq_entry[i].rank_id == rank_id && 
		pq_entry[i].bank_id == bank_id &&
		pq_entry[i].row_id == row_id) {
	  	update_trans_pending_queue(trans_id,i);
		resort_trans_pending_queue(i);
		break;
	}
  }
  if (i>=queue_size) {
	// Add at the end
	pq_entry[i].rank_id = rank_id;
	pq_entry[i].bank_id = bank_id;
	pq_entry[i].row_id = row_id;
	update_trans_pending_queue(trans_id,i);
	queue_size++;
  }

}

void PendingQueue::update_trans_pending_queue(int transaction_id,int location) {
    
	PendingInfo * this_entry = &pq_entry[location];
	  
		/* Add after the head */
	  this_entry->transaction_list[this_entry->transaction_count] = transaction_id;
	  this_entry->transaction_count++;
}


void PendingQueue::resort_trans_pending_queue(int updated_entry) {
	
	int i;
	PendingInfo entry = pq_entry[updated_entry];
	if (updated_entry+1 < queue_size && pq_entry[updated_entry+1].transaction_count > entry.transaction_count) {
	  i = updated_entry + 1;
	  while (i< queue_size && pq_entry[i].transaction_count > entry.transaction_count) {
		pq_entry[i-1] = pq_entry[i];
		i++;
	  }
	   		
		pq_entry[i-1] = entry;
	}else if (updated_entry -1 >=0 && pq_entry[updated_entry - 1].transaction_count < entry.transaction_count) {
	  i = updated_entry - 1;
	  while (i>= 0 && pq_entry[i].transaction_count < entry.transaction_count) {
		pq_entry[i+1] = pq_entry[i];
		i--;
	  }

	  pq_entry[i+1] = entry;

	}
	
}


