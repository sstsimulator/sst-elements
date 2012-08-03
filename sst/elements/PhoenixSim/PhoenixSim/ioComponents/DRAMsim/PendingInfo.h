#ifndef PENDINGINFO__H
#define PENDINGINFO__H

#include "constants.h"

class PendingInfo {

public:
	PendingInfo(){};

 int rank_id;
  int bank_id;
  int row_id;
  int transaction_count;
  int transaction_list[MAX_TRANSACTION_QUEUE_DEPTH];
  int head_location;
  int tail_location;
};

#endif



