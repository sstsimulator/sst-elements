#ifndef COMMAND__H
#define COMMAND__H

//#include <stdint.h>
#include "constants.h"


class Command {

public:
	Command();

	int		status;
	tick_t		start_time;			/* In DRAM ticks */
   	int             command;                        /* which command is this? */
	int		chan_id;
	int 	rank_id;
	int		bank_id;
	int		row_id;
	int		col_id;	/* column address */
	tick_t	completion_time;
	/** Added list of completion times in order to clean up code */
	tick_t	start_transmission_time;
	tick_t	link_comm_tran_comp_time;
	tick_t	amb_proc_comp_time;
	tick_t	dimm_comm_tran_comp_time;
	tick_t	dram_proc_comp_time;
	tick_t	dimm_data_tran_comp_time;
	tick_t	amb_down_proc_comp_time;
	tick_t	link_data_tran_comp_time;
	tick_t	rank_done_time;
	/* Variables added for the FB-DIMM */
	int		bundle_id;			/* Bundle into which command is being sent - Do we need this ?? */
	uint64_t 	tran_id;  			/*	The transaction id number */
	int		data_word;		/* Indicates which portion of data is returned i.e. entire cacheline or fragment thereof
									and which portions are being sent*/	
	int 	data_word_position;	/** This is used to determine which part of the data transmission we are doing : postions include 
								  FIRST , MIDDLE, LAST **/
	int		refresh;		/** This is used to determine if the ras/prec are part of refresh **/
	Command *rq_next_c; /* This is used to set up the refresh queue */
	int		posted_cas;		/** This is used to determine if the ras + cas were in the same bundle **/
	 int     cmd_chked_for_scheduling;   /** This is used to keep track of the latency - set if the command has been checked to be scheudled*/
	int     cmd_updated;    /** This is set if the update_command was called and reqd **/
	int     cmd_id; /* Id added to keep track of which command you have already issued in a list of commands**/
			 
	Command *next_c;





/************** implemented ************/
void print_command_detail(tick_t);
void print_command(tick_t);
tick_t get_command_completion_time() ;
/******************************************/


};

#endif



