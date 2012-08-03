
#ifndef BIU__H
#define BIU__H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>

#include "DRAM_config.h"
#include "BIU_slot.h"
#include "constants.h"
#include "Aux_Stat.h"



class BIU {

public:

	BIU(DRAM_config* cfg);

   tick_t		current_cpu_time;

	int     active_slots;
	int     critical_word_rdy_slots;
	BIU_slot	slot[MAX_BUS_QUEUE_DEPTH];
	int		fixed_latency_flag;
	tick_t		delay;		/* How many CPU delay cycles should the bus queue add? */
							/* debug flags turns on debug printf's */
	int		debug_flag;
	int		access_count[MEMORY_ACCESS_TYPES_COUNT];
	int		proc_request_count[MAX_PROCESSOR];

							/* where to dump out the stats */
	FILE 		*biu_stats_fileptr;
	FILE        *biu_trace_fileptr;

	int		prefetch_biu_hit;		/* How many were eventually used? */
	int		prefetch_entry_count;		/* how many prefetches were done? */

	tick_t          current_dram_time;              /* this is what biu expects the DRAM time to be based on its own computations, not the
							real DRAM time. The real DRAM time should be the same as this one, but it's kept inthe dram system */

	int             thread_count;                   /* how many threads are there on the cpu? */

	int		max_write_burst_depth;		/* If we're doing write sweeping, limit to this number of writes*/
	int		write_burst_count;		/* How many have we done so far?*/

	int     last_bank_id;
	int     last_rank_id;
	int     last_transaction_type;
	tick_t  last_transaction_time;

	DRAM_config  *dram_system_config; /* A pointer to the DRAM system configuration is kept here */
	Aux_Stat *aux_stat;



	void init_biu();

	void set_dram_chan_count_in_biu(int);
	void set_dram_chan_width_in_biu(int);
	void set_dram_cacheline_size_in_biu( int);
	void set_dram_rank_count_in_biu(int);
	void set_dram_bank_count_in_biu(int);
	void set_dram_row_count_in_biu(int);
	void set_dram_col_count_in_biu(int);
	void set_dram_address_mapping_in_biu(int);

	int  get_biu_depth();
	void set_biu_delay(int);
	int  get_biu_delay();

	int  biu_hit_check( int, unsigned int, int, int, tick_t);	/* see if request is already in biu. If so, then convert transparently */
	int  find_free_biu_slot(int);

	int fill_biu_slot(int, int, unsigned int, int, int, void *, RELEASE_FN_TYPE);	/* slot_id, thread_id, now, rid, block address */


	RELEASE_FN_TYPE biu_get_callback_fn(int slot_id) ;

	void *biu_get_access_sim_info(int slot_id) ;
	void release_biu_slot(int);
	int  get_next_request_from_biu();				/* find next occupied bus slot */
	int  get_next_request_from_biu_chan(int);				/* find next occupied bus slot */
	int  bus_queue_status_check( int);
	int  dram_update_required(tick_t);
	int  find_critical_word_ready_slot( int);
	int  find_completed_slot( int, tick_t);
	int  num_free_biu_slot();
	void squash_biu_entry_with_rid(int);

	int  get_rid(int);
	int  get_access_type( int);
	tick_t get_start_time(int);
	unsigned int get_virtual_address(int);
	unsigned int get_physical_address(int);
	void set_critical_word_ready( int);
	int  critical_word_ready(int);
	int  callback_done(int);
	void biu_invoke_callback_fn(int);

	void set_biu_slot_status(int, int);
	int  get_biu_slot_status(int);

	void set_biu_debug(int);
	int  biu_debug();
	void set_biu_fixed_latency(int);
	int  biu_fixed_latency();

	void print_access_type(int,FILE *);
	void print_biu();
	void print_biu_access_count(FILE *);
	void set_current_cpu_time(tick_t);				/* for use with testing harness */

	tick_t get_current_cpu_time();



	void set_last_transaction_type(int);
	int get_last_transaction_type();
	int  get_previous_transaction_type();
	int get_biu_queue_depth ();

	int next_RBRR_RAS_in_biu(int rank_id, int bank_id);
	/* cms specific stuff */

	int  get_thread_id( int);
	void set_thread_count(int);
	int  get_thread_count();

	void scrub_biu(int);


	bool is_biu_busy();

	void gather_biu_slot_stats();



};

#endif

