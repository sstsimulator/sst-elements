#ifndef AUX_STAT__H
#define AUX_STAT__H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

#include "constants.h"
#include "Command.h"
#include "DRAM_config.h"


typedef struct STAT_BUCKET {

	int                     count;
	int                  	delta_stat;		/* latency for bus stat, time between events for DRAM stat */
	struct  STAT_BUCKET 	*next_b;
} STAT_BUCKET;

typedef struct DRAM_STATS {

	uint64_t size; // size of array
	uint64_t* value; // Keeps everything in a large array

	STAT_BUCKET 	*next_b; // if value overflows from array you place it in the stat bucket!
} DRAM_STATS ;

class Aux_Stat {
public:

	Aux_Stat(DRAM_config* cfg);
	~Aux_Stat();
	/* statistical gathering stuff */
	/* where to dump out the stats */
	FILE		*stats_fileptr;
	char* filename;

	DRAM_config *config;


	DRAM_STATS	*bus_stats;
	DRAM_STATS	*bank_hit_stats;
	DRAM_STATS	*bank_conflict_stats;
	DRAM_STATS	*cas_per_ras_stats;
	DRAM_STATS	*biu_access_dist_stats;


	// You need an array for these stats
	int	*tran_queue_valid_stats;
	int *biu_slot_valid_stats;
	int*bundle_stats;

	int		page_size;		/* how big is a page? */
    int     locality_range;
	int		valid_entries;
   	int     locality_hit[CMS_MAX_LOCALITY_RANGE];
	uint64_t	previous_address_page[CMS_MAX_LOCALITY_RANGE];

	/** additional bundle stats for FBD **/
    int num_cas[3];
    int num_cas_w_drive[3];
    int num_cas_w_prec[3];
    int num_cas_write[3];
    int num_cas_write_w_prec[3];
    int num_ras[3];
    int num_ras_refresh[3];
    int num_ras_all[3];
    int num_prec[3];
    int num_prec_refresh[3];
    int num_prec_all[3];
    int num_refresh[3];
    int num_drive[3];
    int num_data[3];
    int num_refresh_all[3];
    /** end additional bundle stats **/



	///stats functions - implmemented
	void mem_gather_stat_init(int, int, int);
	void mem_gather_stat(int, int);
	void mem_print_stat(int,bool);
	void mem_close_stat_fileptrs();
	void init_extra_bundle_stat_collector();
	void gather_extra_bundle_stats(Command *[], int);
	void print_extra_bundle_stats(bool);
	void mem_stat_init_common_file(char *);
	FILE *get_common_stat_file();
	void print_rbrr_stat(int);
	void print_bank_conflict_stats();
	void free_stats(STAT_BUCKET *start_b);

};

#endif
