#ifndef DRAM_CONFIG__H
#define DRAM_CONFIG__H

#include <stdio.h>
#include <stdlib.h>

#include "UpdateInterface.h"
#include "Global_TQ_info.h"




class DRAM_config {


public:

	DRAM_config();


	int		cpu_frequency;			/* cpu frequency now kept here */
	double 		mem2cpu_clock_ratio;
	double		cpu2mem_clock_ratio;
	int		dram_type;					/* SDRAM, DDRSDRAM,  etc. */
	int		memory_frequency;			/* clock freq of memory controller system, in MHz */
	int		dram_frequency;				/* FB-DIMM only clock freq of the dram system, in MHz */
	float 	memory2dram_freq_ratio;		/* FBD-DIMM multiplier for timing */
	uint64_t		dram_clock_granularity;		/* 1 for SDRAM, 2 for DDR and etc */
	int		critical_word_first_flag;	/* SDRAM && DDR SDRAM should have this flag set, */

	int		physical_address_mapping_policy;/* which address mapping policy for Physical to memory? */
    int     row_buffer_management_policy;   /* which row buffer management policy? OPEN PAGE, CLOSE PAGE, etc */
    int     refresh_policy;   				/* What refresh policies are applied */
    int     refresh_issue_policy;   				/* Should we issue at once or when opportunity arises*/

	int		cacheline_size;			/* size of a cacheline burst, in bytes */
	int		channel_count;			/* How many logical channels are there ? */
	int		channel_width;			/* How many bytes width per logical channel? */
	int		up_channel_width;	/* FB-DIMM : Up channel width in bits */
	int		down_channel_width;	/* FB-DIMM : Down channel width in bits */
	int 	rank_count;			/* How many ranks are there on the channel ? */
	int		bank_count;			/* How many banks per device? */
	int		row_count;			/* how many rows per bank? */
	int		col_count;			/* Hwo many columns per row? */
      int     packet_count;           /*   cacheline_size / (channel_width * 8) */

	int max_tq_size;
							/* we also ignore t_pp (min prec to prec of any bank) and t_rr (min ras to ras).   */
	int		data_cmd_count;		/** For FBDIMM : Reqd to determine how many data "commands" need to be sent **/
	int		drive_cmd_count;		/** For FBDIMM : Reqd to determine how many drive "commands" need to be sent currently set to 1**/
	int     	t_burst;          		/* number of cycles utilized per cacheline burst */
	int		t_cas;				/* delay between start of CAS command and start of data burst */
	int		t_cmd;				/* command bus duration... */
	int		t_cwd;				/* delay between end of CAS Write command and start of data packet */
	int		t_cac;				/* delay between end of CAS command and start of data burst*/
	int		t_dqs;				/* rank hand off penalty. 0 for SDRAM, 2 for DDR, and */
	uint64_t		t_faw;				/* four bank activation */
	int		t_ras;				/* interval between ACT and PRECHARGE to same bank */
	int		t_rc;				/* t_rc is simply t_ras + t_rp */
	int		t_rcd;				/* RAS to CAS delay of same bank */
	uint64_t	t_rrd;				/* Row to row activation delay */
	int		t_rp;				/* interval between PRECHARGE and ACT to same bank */
	int		t_rfc;				/* t_rfc -> refresh timing : for sdram = t_rc for ddr onwards its t_rfc (refer datasheetss)*/
	int		t_wr;				/* write recovery time latency, time to restore data o cells */
	int		t_rtp;				/* Interal READ-to-PRECHARGE delay - reqd for CAS with PREC support prec delayed by this amount */

	int		posted_cas_flag;		/* special for DDR2 */
	int		t_al;				/* additive latency = t_rcd - 2 (ddr half cycles) */

	int		t_rl;				/* read latency  = t_al + t_cas */
	int		t_rtr;				/* delay between start of CAS Write command and start of write retirement command*/

	int		t_bus;					/* FBDIMM - bus delay */
	int		t_amb_up;				/* FBDIMM - Amb up delay  - also the write buffer delay*/
	int		t_amb_down;				/* FBDIMM - Amb down delay - also the read buffer delay  Note that this implies the first 2 bursts are packaged and sent down in this time later -> overlap of dimm and link burst is seen*/
	int		t_amb_forward;				/* FBDIMM - Amb to AMB forwarding delay */
    uint64_t     t_bundle;          		/* FBDIMM number of cycles utilized to transmit a bundle */
	int		up_buffer_cnt;			/* FBDIMM	number of buffers available at the AMB for write cmds  1 buffer => 1 cache line*/
	int		down_buffer_cnt;		/* FBDIMM	number of buffers available at the AMB for read cmds */
	int		var_latency_flag;		/* FBDIMM	Set when the bus latency is to not rank dependent */
	int		row_command_duration;
	int		col_command_duration;
    int     auto_refresh_enabled;       /* interleaved autorefresh, or no refresh at all */

	double		refresh_time;			/* given in microseconds. Should be 64000. 1 us is 1 MHz, makes the math easier */
	int		refresh_cycle_count;		/* Every # of DRAM clocks, send refresh command */
	int 	watch_refresh;
	uint64_t ref_tran_id;

	tick_t arrival_threshold;
	int		strict_ordering_flag;		/* if this flag is set, we cannot opportunistically allow later */
							/* CAS command to by pass an earlier one. (earlier one may be waiting for PREC+RAS) */
							/* debug flags turns on debug printf's */
	int		dram_debug_flag;
	int		addr_debug_flag;			/* debug the address mapping schemes */
	int		wave_debug_flag;			/* draw cheesy waveforms */
	int		issue_debug_flag;			/* Prints out when things got issued - DDRX */
	int		wave_cas_debug_flag;			/* draw cheesy waveforms of cas activity only*/
	int		bundle_debug_flag;			/* for FBDIMM */
	int		amb_buffer_debug_flag;		/* for FBDIMM */

	int		tq_delay;				/* How many DRAM delay cycles should the memory controller add for delay through the transaction queue? */

	bool		single_rank; // true - Number of ranks per dimm=1 else false for dual rank systems
	int 		num_thread_sets;
	int		thread_sets[32]; // need to make this some const
	int 		thread_set_map[32];

	bool    	cas_with_prec; // For close page systems this will be used

	int             transaction_selection_policy;
	int		bus_queue_depth;

  	UpdateInterface	 *dram_power_config;	/* Power Configuration */
	Global_TQ_info   tq_info;





/**********************************************************************************************************************/
/******************************** function prototypes *****************************************************************/

/******** implemented *********/
void init_dram_system_configuration();
void convert_config_dram_cycles_to_mem_cycles();
void set_chipset_delay(int);
int  get_chipset_delay();
void set_dram_type(int);
void set_dram_frequency(int);
int  get_dram_frequency();
int  get_dram_type();
void set_memory_frequency(int);
int  get_memory_frequency();
void set_dram_rank_count(int rank_count);
void set_dram_bank_count(int);
void set_dram_row_count(int);
void set_dram_col_count(int);
void set_dram_channel_count(int);
int get_dram_channel_count();
int get_dram_rank_count();
int get_dram_bank_count();
int get_dram_row_count();
int get_dram_col_count();
int get_dram_row_buffer_management_policy();
void set_dram_buffer_count(int);
void set_dram_channel_width(int);
void set_dram_up_channel_width(int); /*** FB-DIMM ***/
void set_dram_down_channel_width(int); /*** FB-DIMM ***/
void set_dram_transaction_granularity(int);
void set_pa_mapping_policy(int);
void set_dram_row_buffer_management_policy(int);
void set_dram_refresh_policy(int);
void set_dram_refresh_issue_policy(int);
void set_dram_refresh_time(int);
void set_fbd_var_latency_flag(int);
void set_posted_cas(bool);
void set_dram_debug(int);
void set_addr_debug(int);
void set_wave_debug(int);
void set_issue_debug(int);
void set_wave_cas_debug(int);
void set_bundle_debug(int);
void set_amb_buffer_debug(int);
void set_tran_watch(uint64_t tran_id);
int get_tran_watch(uint64_t tran_id);
int get_ref_tran_watch(uint64_t tran_id);
void set_ref_tran_watch(uint64_t tran_id);
void enable_auto_refresh(int, int);
 int  dram_debug();
 int  addr_debug();
int  wave_debug();
 int  cas_wave_debug();
 int  bundle_debug();
 int  amb_buffer_debug();
void set_strict_ordering(int);
void set_t_bundle(int);
void set_t_bus(int);
void set_independent_threads();

void print_dram_type( int dram_type, FILE *stream );
void print_pa_mapping_policy( int papolicy, FILE *stream );
void print_row_policy( int rowpolicy, FILE *stream );
void print_refresh_policy( int refreshpolicy, FILE *stream );
void print_trans_policy( int policy, FILE *stream );
void dump_config(FILE* stream);

void set_cpu_memory_frequency_ratio();
void set_cpu_frequency(int);
int  get_cpu_frequency();

void set_transaction_selection_policy(int);
int  get_transaction_selection_policy();

int get_num_read();
int get_num_write();
int get_num_ifetch();
int get_num_prefetch();
void set_transaction_debug(int debug_status);
void set_debug_tran_id_threshold(uint64_t dtit);

int get_biu_queue_depth ();
void set_biu_depth(int depth);

/***********************************/




};


#endif

