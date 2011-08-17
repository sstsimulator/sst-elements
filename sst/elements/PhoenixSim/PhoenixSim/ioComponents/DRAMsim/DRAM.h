#ifndef DRAM__H
#define DRAM__H

#include "Memory_Controller.h"
#include "DRAM_config.h"
#include "PowerConfig.h"
#include "Aux_Stat.h"

#include "constants.h"
#include "BIU.h"

class CPU_CALLBACK{
public:
	virtual void return_transaction( int thread_id, int trans_id) = 0;
};


class DRAM {

public:

	DRAM();
	DRAM(BIU* b, DRAM_config* cnfg, Aux_Stat* st);


	tick_t				current_dram_time;
	tick_t				last_dram_time;
	tick_t				current_mc_time;
	tick_t				last_mc_time;
	DRAM_config*			 	config;
	PowerConfig*			power_config;
	Memory_Controller**       		dram_controller;
	///BIU* 				biu;
	Aux_Stat*			aux_stat;



CPU_CALLBACK *callback;   //interface to return completed transactions


void init_dram_system();				/* set DRAM to system defaults */

int update_dram(tick_t);
bool is_dram_busy();
tick_t get_dram_current_time();
void convert_config_dram_cycles_to_mem_cycles();
DRAM_config *get_dram_system_config();




void print_transaction_queue();





void retire_transactions();


void gather_tran_stats();
void gather_biu_slot_stats();

bool schedule_new_transaction(int thread_id, uint64_t address, int trans_type, uint64_t rid);




};

#endif

