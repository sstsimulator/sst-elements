#ifndef TRANSACTIONQUEUE__H
#define TRANSACTIONQUEUE__H

#include "Transaction.h"
#include "Command.h"
#include "DRAMaddress.h"
#include "DRAM_config.h"
#include "Global_TQ_info.h"
#include "BIU.h"
#include "Bank.h"

class TransactionQueue {

public:
	TransactionQueue(Global_TQ_info* inf, DRAM_config* cfg, Aux_Stat* st);

	int		transaction_count;		/** Tail at which we add **/
	Global_TQ_info* tq_info;
	Transaction 	entry[MAX_TRANSACTION_QUEUE_DEPTH];	/* FIFO queue inside of controller */
	DRAM_config*	config;
	Aux_Stat*	aux_stat;

	long long rank_distrib[8][8];
	long long tot_reqs;


/********* implemented *********/

void init_transaction_queue();
void set_chipset_delay(int);
int  get_chipset_delay();

Command *add_command(tick_t, Command *, int, DRAMaddress *, uint64_t,int,int,int);
void release_command(Command *);
Command *acquire_command();
Transaction*  add_transaction(tick_t, int, int,uint64_t,DRAMaddress *a, Bank* this_b );
void remove_transaction(int tindex, int current_dram_time);
Command *transaction2commands(tick_t, int, int, DRAMaddress *, Bank* this_b);


void print_bundle(tick_t,Command *[],int);
void print_transaction(tick_t,int,uint64_t);

int get_transaction_debug(void);

void  collapse_cmd_queue( Transaction * this_t, Command *cmd_issued);
int get_transaction_index(uint64_t);
/****************************/


int  virtual_address_translation(int, int, DRAMaddress *);		/* take virtual address, map to physical address */
int  physical_address_translation(int, DRAMaddress *);		/* take physical address, convert to channel, rank, bank, row, col addr */
void print_addresses(DRAMaddress *);
int convert_address(int , DRAM_config *, DRAMaddress *);



void commands2bundle(tick_t, int,char *);





int  transaction_debug();



  








};
#endif


