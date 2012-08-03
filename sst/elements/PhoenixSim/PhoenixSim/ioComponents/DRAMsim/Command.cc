#include "Command.h"
#include <stdio.h>
#include <stdlib.h>

Command::Command(){

	rq_next_c = NULL;
	rank_done_time = 0;


	start_time=0;

		chan_id =0;
		rank_id =0;
		bank_id =0;
		row_id =0;
		col_id =0;	/* column address */
		completion_time =0;
		/** Added list of completion times in order to clean up code */
		start_transmission_time =0;
		link_comm_tran_comp_time =0;
		amb_proc_comp_time =0;
		dimm_comm_tran_comp_time =0;
		dram_proc_comp_time =0;
		dimm_data_tran_comp_time =0;
		amb_down_proc_comp_time =0;
		link_data_tran_comp_time =0;
		rank_done_time =0;

}




void Command::print_command_detail(tick_t now){

	print_command(now);
	fprintf(stdout,"LINKCT		%llu\n",link_comm_tran_comp_time);
	fprintf(stdout,"AMBPROC		%llu\n",amb_proc_comp_time);
	fprintf(stdout,"DIMMCT		%llu\n",dimm_comm_tran_comp_time);
	fprintf(stdout,"DRAMPROC	%llu\n",dram_proc_comp_time);
	fprintf(stdout,"DIMMDT		%llu\n",dimm_data_tran_comp_time);
	fprintf(stdout,"AMBDOWN     %llu\n",amb_down_proc_comp_time);
	fprintf(stdout,"LINKDT		%llu(%d)\n",link_data_tran_comp_time,data_word);
	fprintf(stdout,"Completion	%llu\n",completion_time);

}


void Command::print_command(tick_t now){

    fprintf(stdout," NOW[%llu] status[%d] end[%d] ", now, status, (int) completion_time);
    if(command == PRECHARGE) {
      fprintf(stdout,"PRECHARGE ");
    } else if(command == RAS) {
      fprintf(stdout,"RAS       ");
    } else if(command == CAS) {
      fprintf(stdout,"CAS       ");
    } else if(command == CAS_WITH_DRIVE) {
      fprintf(stdout,"CAS_WITH_DRIVE ");
    } else if(command == CAS_WRITE) {
      fprintf(stdout,"CAS WRITE ");
    } else if(command == CAS_AND_PRECHARGE) {
      fprintf(stdout,"CAP		  ");
    } else if(command == CAS_WRITE_AND_PRECHARGE) {
      fprintf(stdout,"CPW       ");
    } else if(command == DATA ) {
      fprintf(stdout,"DATA 	   ");
    } else if(command == DRIVE ) {
      fprintf(stdout,"DRIVE 	  ");
    } else if(command == RAS_ALL) {
      fprintf(stdout,"RAS_ALL   ");
    } else if(command == PRECHARGE_ALL) {
      fprintf(stdout,"PREC_ALL  ");
    } else if(command == REFRESH ) {
      fprintf(stdout,"REFRESH   ");
    } else if(command == REFRESH_ALL ) {
      fprintf(stdout,"REF_ALL   ");
    } else {
      fprintf(stdout,"UNKNOWN[%d] ", command);
    }
    fprintf(stdout,"chan_id[%2X] rank_id[%2X] bank_id[%2X] row_id[%5X] col_id[%4X] tran_id[%llu] cmd_id[%d] ref[%d]\n",
	    chan_id,
	    rank_id,
	    bank_id,
	    row_id,
	    col_id,
	    tran_id,
		cmd_id,
        refresh);

}

tick_t Command::get_command_completion_time() {
  return rank_done_time;
}

