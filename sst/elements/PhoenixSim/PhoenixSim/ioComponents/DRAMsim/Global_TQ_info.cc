#include "Global_TQ_info.h"



Global_TQ_info::Global_TQ_info(){

	 debug_flag		= FALSE;
 	 debug_tran_id_threshold = 0;
 	 tran_watch_flag	= FALSE;
  	tran_watch_id = 0;

	transaction_id_ctr = 0;
	num_ifetch_tran = 0;
	num_read_tran = 0;
	num_prefetch_tran = 0;
	num_write_tran = 0;

	free_command_pool		= 0;

}




int Global_TQ_info::get_transaction_debug(){	
  return (debug_flag && 
	  (transaction_id_ctr >= debug_tran_id_threshold));
}

