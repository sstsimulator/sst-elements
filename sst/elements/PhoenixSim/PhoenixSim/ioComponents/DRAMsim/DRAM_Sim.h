#ifndef DRAM_SIM__H
#define DRAM_SIM__H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <math.h>

#include "Aux_Stat.h"
#include "BusEvent.h"
#include "constants.h"
#include "DRAM_config.h"
#include "Command.h"
#include "TransactionQueue.h"
#include "BIU.h"
#include "DRAM.h"
//#include "MemRequest.h"


enum MemOperation {
  MemRead = 0,
  MemWrite,
  MemPush,
  MemIfetch,    	//added by gilbert because dramsim supports it
  MemPrefetch,  	//added by gilbert because dramsim supports it
  MemReadW,                  // store that misses and needs data,
                             // never comes from the processor, it is
                             // always converted by a cache from a MemWrite
  MemLastOp // just to get the size of the enumeration
};


class DRAM_Sim {

public:
	DRAM_Sim(BIU* b, DRAM_config *con);

	int type;
	enum SIM_TYPE {
		TRACE, RAND, OMNET
	};

	int numSystems;
	int threadCount;

	BIU* biu;
	DRAM* dram;
	DRAM_config* config;
	Aux_Stat *aux_stat;

	CPU_CALLBACK *callback;   //interface to return completed transactions

	tick_t current_cpu_time;
	tick_t max_cpu_time;
	time_t sim_start_time;
	time_t sim_end_time;

	int transaction_total;
	int transaction_retd_total;
	int last_retd_total;

	bool biu_stats;
	bool biu_slot_stats;
	bool biu_access_dist_stats;
	bool bundle_stats;
	bool tran_queue_stats;
	bool bank_hit_stats;
	bool bank_conflict_stats;
	bool cas_per_ras_stats;

	int getChannelWidth(){ return config->channel_width ;}

	void setCallback(CPU_CALLBACK* cb){ callback = cb; dram->callback = cb; };

	virtual void start() {
	}
	virtual bool update() {
		//biu->set_current_cpu_time((tick_t) current_cpu_time);
		//if (biu->bus_queue_status_check(MEM_STATE_INVALID) == BUSY) {

			return dram->update_dram(current_cpu_time++);

		//}

		///current_cpu_time++;
		/* look for loads that have already returned the critical word */
		/*int slot_id;
		//fprintf(stdout, "thread_count: %d", biu->get_thread_count());
		for (int i = 0; i < biu->get_thread_count(); i++) {
			slot_id = biu->find_critical_word_ready_slot(i);
			if (slot_id != MEM_STATE_INVALID) {
				//fprintf(stdout, "DRAM: ciritcal word\n");
				if(callback != NULL){
					int thread_id = biu->get_thread_id(slot_id);
					int trans_id = biu->get_rid(slot_id);
					int addr = biu->get_virtual_address(slot_id);

					callback->return_transaction(thread_id, trans_id);
				}
			} else {
				// find slots that's ready to retire, and retire it
				slot_id = biu->find_completed_slot(i, (tick_t) current_cpu_time);
				if (slot_id != MEM_STATE_INVALID) {
					//fprintf(stdout, "DRAM: retiring\n");
					biu->release_biu_slot(slot_id);
					transaction_retd_total++;
				}
			}
		}*/

		///return biu->active_slots > 0;

	} //update dram with latest clock value

	virtual bool update(tick_t clk) {

		current_cpu_time = clk;


		///biu->set_current_cpu_time((tick_t) current_cpu_time);
		///if (biu->bus_queue_status_check(MEM_STATE_INVALID) == BUSY) {

			return dram->update_dram(current_cpu_time);

		///}



		// look for loads that have already returned the critical word
		/*int slot_id;
		//fprintf(stdout, "thread_count: %d", biu->get_thread_count());
		for (int i = 0; i < biu->get_thread_count(); i++) {
			slot_id = biu->find_critical_word_ready_slot(i);
			if (slot_id != MEM_STATE_INVALID) {
				//fprintf(stdout, "DRAM: ciritcal word\n");
				if(callback != NULL){
					int thread_id = biu->get_thread_id(slot_id);
					int trans_id = biu->get_rid(slot_id);
					int addr = biu->get_virtual_address(slot_id);

					callback->return_transaction(thread_id, trans_id);
				}
			} else {
				// find slots that's ready to retire, and retire it
				slot_id = biu->find_completed_slot(i, (tick_t) current_cpu_time);

				if (slot_id != MEM_STATE_INVALID) {
					//fprintf(stdout, "DRAM: retiring\n");
					biu->release_biu_slot(slot_id);


					transaction_retd_total++;
				}
			}
		}*/



		///return biu->active_slots > 0;

	} //update dram with latest clock value

	void setcallback(CPU_CALLBACK* c) {callback = c; dram->callback = c;}
	virtual void init();
	virtual void finish();
	virtual bool add_new_transaction(int thread_id, int trans_id,
			unsigned int addr, int access_type, int priority) {
		/* place request into BIU */
		///int slot = biu->fill_biu_slot(thread_id,  trans_id, addr,
		///		access_type, priority, NULL, NULL);
		///fprintf(stdout, " filled slot %d\n", slot);
		///return slot;

		return dram->schedule_new_transaction(thread_id, addr, access_type, trans_id);
	}
	; //add new trans to dram

	void set_thread_count(int c) {threadCount = c; biu->set_thread_count(c);}

	static DRAM_Sim* read_config(double penis, int argc, char *argv[], int nothing);

	static int file_io_token(char *input);
	static void read_dram_config_from_file(FILE *fin, DRAM_config *this_c);

	static void read_power_config_from_file(FILE * fp,
			PowerConfig * power_config_ptr);


};

class DRAM_Sim_TRACE: public DRAM_Sim {

public:
	DRAM_Sim_TRACE(BIU* b, DRAM_config *con, int num, char** files);

	virtual ~DRAM_Sim_TRACE();

	int num_trace;
	BusEvent* current_bus_event;
	char **trace_filein;
	FILE *trace_fileptr[MAX_TRACES];

	virtual void start();

	BusEvent *get_next_bus_event(const int num_trace);

	int trace_file_io_token(char *input);

};

class DRAM_Sim_RAND: public DRAM_Sim {

public:
	DRAM_Sim_RAND(BIU* b, DRAM_config *con, int type, float access_dist[4]);

	double average_interarrival_cycle_count; /* every X cycles, a transaction arrives */
	double arrival_thresh_hold; /* if drand48() exceed this thresh_hold. */
	int arrival_distribution_model;
	float *access_distribution; /* used to set distribution precentages of access types */

	virtual void start();

	/** gaussian number generator ganked from the internet ->
	 * http://www.taygeta.com/random/gaussian.html
	 */

	double box_muller(double m, double s);

	double gammaln(double xx);
	/**
	 * from the book "Numerical Recipes in C: The Art of Scientific Computing"
	 **/

	double poisson_rng(double xm);

	int get_mem_access_type(float *access_distribution);
};

class DRAM_Sim_MONET: public DRAM_Sim {

public:

	DRAM_Sim_MONET(BIU* biu, DRAM_config* cfg);

	virtual void start();

};

#endif

