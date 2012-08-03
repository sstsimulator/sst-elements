#ifndef MEMORY_CONTROLLER__H
#define MEMORY_CONTROLLER__H

#include "TransactionQueue.h"
#include "Bus.h"
#include "DIMM.h"
#include "Rank.h"
#include "RBRR.h"
#include "PendingQueue.h"
#include "RefreshQueue.h"
#include "Command.h"
#include "DRAMaddress.h"


class Memory_Controller {


public:

	Memory_Controller(DRAM_config* cfg, Aux_Stat *st, int i);

	int			id;
    	TransactionQueue     	*transaction_queue;
	Rank 			rank[MAX_RANK_COUNT];  		/*  device-bank in SDRAM system */
	DIMM 			dimm[MAX_RANK_COUNT];  		/* assume 1 per rank initially: used in fb-dimm system */
	Bus			command_bus;			/* used for DDR/SDRAM */
	Bus			data_bus;			/* used for every one with bi-directional bus */
	Bus			row_command_bus;		/* used for systems that use separate row and col command busses */
	Bus			col_command_bus;

	/* Bus Descriptions added for FB-DIMM	*/
	Bus			up_bus;					/* point-to-point link to the DRAMs -> carries cmd and write data */
	Bus			down_bus;				/* point-to-point link from the DRAMs -> carries read data  */
	uint64_t 		bundle_id_ctr;			/* Keeps track of how manyth bundle in the channel is to be issued*/			
	int     		last_rank_id;
	int     		last_bank_id;
	int     		last_command;
	int     		next_command;
	int 			t_faw_violated;
	int			t_rrd_violated;
	int			t_dqs_violated;	

	/* We need some stuff for the RBRR */
	RBRR  			sched_cmd_info;
	RefreshQueue		refresh_queue;
	PendingQueue		pending_queue;
	int 			active_write_trans;
	bool			active_write_flag;
	int 			active_write_burst;

	DRAM_config*		config;
	Aux_Stat* 		aux_stat;


/*****    implemented ***/

void init_dram_controller(int);

int  row_command_bus_idle();
int  col_command_bus_idle();
int is_cmd_bus_idle(Command *this_c);
int  up_bus_idle();
int  down_bus_idle();
void set_row_command_bus(int, int);
void set_col_command_bus(int, int);


bool can_issue_command(int, Command *, int);
bool can_issue_cas(int, Command *, int);
bool can_issue_ras(int, Command *, int);
bool can_issue_prec(int, Command *, int);
bool can_issue_prec_all(int, Command *, int);
bool can_issue_ras_all(int, Command *, int);


bool can_issue_cas_with_precharge(int, Command *, int);

bool check_for_refresh(Command * this_c,int tran_index,tick_t cmd_bus_reqd_time, int current_dram_time);

int update_command_states(tick_t,int, int,char *);
void precharge_transition(Command *, int, int, int, int, int, char *, int);
void precharge_all_transition(Command *, int, int, int, int, int, char *, int);
void refresh_transition(Command *, int, int, int, int, int, char *, int);
void refresh_all_transition(Command *, int, int, int, int, int, char *, int);
void cas_transition(Command *, int, int, int, int, int, char *, int);
void cas_write_transition(Command *, int, int, int, int, int, char *, int);
void ras_transition(Command *, int, int, int, int, int, char *, int);
void ras_all_transition(Command *, int, int, int, int, int, char *, int);
void data_transition(Command *, int, int, int, int, int, char *, int);

void get_reqd_times(Command * this_c,tick_t *cmd_bus_reqd_time,tick_t *dram_reqd_time, int current_dram_time);


bool check_cmd_bus_required_time (Command * this_c, tick_t cmd_bus_reqd_time);
bool check_data_bus_required_time (Command * this_c, tick_t data_bus_reqd_time);
bool check_down_bus_required_time (Command * this_c, Command *temp_c,tick_t down_bus_reqd_time);

void remove_refresh_transaction(Command *prev_rq_c, Command* this_rq_c, int time);
void update_refresh_status(tick_t now,char * debug_string) ;

void update_refresh_missing_cycles (tick_t now,char * debug_string) ;

void update_bus_link_status (int current_dram_time);

DRAMaddress get_next_refresh_address ();

bool is_refresh_pending();



int check_cke_hi_pre( int rank_id);
int check_cke_hi_act( int rank_id);
void update_cke_bit(tick_t now, int max_ranks);


int can_issue_refresh_command(tick_t now, Command *this_rq_c, Command *this_c) ;
int can_issue_refresh_ras(Command* this_rq_c, Command *this_c);
int can_issue_refresh_prec(tick_t now, Command* this_rq_c, Command *this_c);
int can_issue_refresh_ras_all(Command* this_rq_c, Command *this_c);
int can_issue_refresh_prec_all(tick_t now, Command* this_rq_c, Command *this_c);
int can_issue_refresh_refresh_all(tick_t now, Command *this_rq_c,Command *this_c); 
int can_issue_refresh_refresh(tick_t now, Command *this_rq_c,Command *this_c); 

Command * get_next_refresh_command_to_issue(tick_t now, int chan_id,int transaction_selection_policy,int command);

Command *next_pre_in_transaction_queue(int transaction_selection_policy, int cmd_type,int chan_id,int rank_id,int bank_id,bool write_burst, int time);
Command *next_cmd_in_transaction_queue(int transaction_selection_policy, int cmd_type,int chan_id,int rank_id,int bank_id,bool write_burst);
void update_rbrr_rank_bank_info(int transaction_selection_policy, int cmd,int rank_id,int bank_id, int cmd_index, int rank_count,int bank_count);

void issue_new_commands(tick_t, char *);
void insert_refresh_transaction(int time);
Command* build_refresh_cmd_queue(tick_t now ,DRAMaddress refresh_address,unsigned int refresh_id);


void issue_cmd(tick_t,Command *, Transaction*);
void issue_cas(tick_t,Command *);
void issue_cas_write(tick_t,Command *);
void issue_ras_or_prec(tick_t,Command *); /** OBSOLETE FUNCTION**/
void issue_ras(tick_t,Command *);
void issue_prec(tick_t,Command *);
void issue_data(tick_t,Command *);
void issue_drive(tick_t,Command *);
void issue_refresh(tick_t ,Command *);


Command * get_next_cmd_to_issue(tick_t now, int trans_sel_policy);
bool is_bank_open(int rank_id,int bank_id, int row_id) ;
void update_critical_word_info (tick_t now,Command * this_c,Transaction *this_t);

void update_power_stats(tick_t now, Command *this_c);

/***********************/



};

#endif

