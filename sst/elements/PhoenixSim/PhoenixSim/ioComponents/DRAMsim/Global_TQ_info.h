#ifndef GLOBAL_TQ_INFO__H
#define GLOBAL_TQ_INFO__H

#include "Command.h"



class Global_TQ_info {

public:
	Global_TQ_info();

	bool debug_flag;
	uint64_t	debug_tran_id_threshold;	/** tran id at which to start debug **/ 
	int     num_ifetch_tran;
    int     num_read_tran;
    int     num_prefetch_tran;
    int     num_write_tran;
	uint64_t		tran_watch_id;
	bool		tran_watch_flag;	/* Set to true if you are indeed watching a transaction **/
	uint64_t		transaction_id_ctr;		/* Transaction id for new transaction **/
	Command		*free_command_pool;
	tick_t last_transaction_retired_time;




	int get_transaction_debug();


};

#endif 


