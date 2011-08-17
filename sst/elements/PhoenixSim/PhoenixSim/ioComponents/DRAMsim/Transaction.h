#ifndef TRANSACTION__H
#define TRANSACTION__H

#include "Command.h"
#include "DRAMaddress.h"


class Transaction {

public:
	Transaction(){next_c = NULL; next_ptr = NULL;};



	int		status;
	tick_t		arrival_time;		/* time when first seen by the memory controller in DRAM clock ticks */
	tick_t		completion_time;	/* time when entire transaction has completed in DRAM clock ticks */
	uint64_t	transaction_id;			/** Gives the Transaction id **/
	int 		thread_id;
	uint64_t		request_id;
	int		transaction_type;	/* command type */
	int		slot_id;		/* Id of the slot that requested this transaction */
	DRAMaddress 	address;
	int		tindex; // To keep track of which slot you are in
	//int		chan_id;		/* id of the memory controller */
    //uint64_t    physical_address;
	int		critical_word_available;	/* critical word flag used in the FBDIMM */
	int		critical_word_ready;	/* critical word ready flag */
	tick_t		critical_word_ready_time;	/* time when the critical word first filters back in DRAM clock ticks */
	Command		*next_c;

	bool issued_command;
	bool issued_data;
	bool issued_cas;
	bool issued_ras; // helps keep track of bank conflicts in case of open page
	Transaction * next_ptr; /* Used by the pending queue only invalid in the actual transaction queue */


	void print_transaction_index(tick_t now, int rid);
	void print_transaction(tick_t now, int rid);

};

#endif



