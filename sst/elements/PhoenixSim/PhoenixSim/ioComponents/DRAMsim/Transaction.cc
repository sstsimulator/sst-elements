#include "Transaction.h"

#include "constants.h"
#include <assert.h>




void Transaction::print_transaction_index(tick_t now,  int rid){

  Command *this_c;

  assert(tindex <= MAX_TRANSACTION_QUEUE_DEPTH);
  if(transaction_type == MEMORY_READ_COMMAND ){
	fprintf(stdout,"READ_TRANSACTION [%d] to addr[%llx] rid(%d)\n",transaction_id,address.physical_address,rid);
  } else if(transaction_type == MEMORY_IFETCH_COMMAND ){
	fprintf(stdout,"IFETCH_TRANSACTION [%d] to addr[%llx] rid(%d)\n",transaction_id,address.physical_address,rid);
  } else if(transaction_type == MEMORY_WRITE_COMMAND ){
	fprintf(stdout,"WRITE_TRANSACTION [%d] to addr[%llx] rid(%d)\n",transaction_id,address.physical_address,rid );
  } else if(transaction_type == MEMORY_PREFETCH ){
	fprintf(stdout,"MEMORY_PREFETCH [%d] to addr[%llx] rid(%d)\n",transaction_id,address.physical_address,rid);
  }
  this_c = next_c;
  while(this_c != NULL){
	this_c->print_command(now);
	this_c = this_c->next_c;
  }
  fprintf(stdout,"------------------\n");
  fflush(stdout);
}


void Transaction::print_transaction(tick_t now, int rid){
  
  Command *this_c;

  assert(tindex <= MAX_TRANSACTION_QUEUE_DEPTH);
  if(transaction_type == MEMORY_READ_COMMAND ){
    fprintf(stdout,"READ_TRANSACTION [%llu] to addr[%llx] rid(%d)\n",transaction_id,address.physical_address,rid);
  } else if(transaction_type == MEMORY_IFETCH_COMMAND ){
    fprintf(stdout,"IFETCH_TRANSACTION [%llu] to addr[%llx] rid(%d)\n",transaction_id,address.physical_address,rid);
  } else if(transaction_type == MEMORY_WRITE_COMMAND ){
    fprintf(stdout,"WRITE_TRANSACTION [%llu] to addr[%llx] rid(%d)\n",transaction_id,address.physical_address,rid );
  } else if(transaction_type == MEMORY_PREFETCH ){
    fprintf(stdout,"MEMORY_PREFETCH [%llu] to addr[%llx] rid(%d)\n",transaction_id,address.physical_address,rid);
  }
  this_c = next_c;
  while(this_c != NULL){
    this_c->print_command(now);
    this_c = this_c->next_c;
  }
  fprintf(stdout,"------------------\n");
  fflush(stdout);
}
