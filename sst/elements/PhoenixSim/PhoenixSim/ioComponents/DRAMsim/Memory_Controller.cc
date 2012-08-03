#include "Memory_Controller.h"

#include <assert.h>
#include "constants.h"
#include "utilities.h"

#include "DRAM.h"

Memory_Controller::Memory_Controller(DRAM_config* cfg, Aux_Stat* st, int i) {

	id = i;
	aux_stat = st;
	config = cfg;
	transaction_queue = new TransactionQueue(&(cfg->tq_info), cfg, st);
	init_dram_controller(id);

}

void Memory_Controller::init_dram_controller(int controller_id) {
	int i, j;

	Rank *this_r;

	/* initialize all ranks */
	for (i = 0; i < MAX_RANK_COUNT; i++) {
		this_r = &(rank[i]);
		for (j = 0; j < MAX_BANK_COUNT; j++) {
			this_r->bank[j].status = IDLE;
			this_r->bank[j].row_id = MEM_STATE_INVALID;
			this_r->bank[j].ras_done_time = 0;
			this_r->bank[j].cas_done_time = 0;
			this_r->bank[j].rcd_done_time = 0;
			this_r->bank[j].rp_done_time = 0;
			this_r->bank[j].last_command = IDLE;
			this_r->bank[j].cas_count_since_ras = 0;
			this_r->bank[j].paging_policy = OPEN_PAGE;
		}
		for (j = 0; j < 4; j++) {
			this_r->activation_history[j] = 0;
		}
	}
	id = controller_id;
	command_bus.status = IDLE;
	data_bus.status = IDLE;
	data_bus.current_rank_id = MEM_STATE_INVALID;
	row_command_bus.status = IDLE;
	col_command_bus.status = IDLE;
	up_bus.status = IDLE;
	up_bus.completion_time = 0;
	down_bus.status = IDLE;
	last_rank_id = config->rank_count - 1;
	last_bank_id = config->bank_count - 1;
	last_command = IDLE;
	next_command = RAS;
	bundle_id_ctr = 0;
	refresh_queue.refresh_row_index = 0;
	refresh_queue.refresh_rank_index = 0;
	refresh_queue.refresh_bank_index = 0;
	refresh_queue.last_refresh_cycle = 0;
	refresh_queue.refresh_pending = FALSE;
	refresh_queue.rq_head = NULL;

	//clear all the variables
	int chan_id, rank_id;
	for (chan_id = 0; chan_id < MAX_CHANNEL_COUNT; chan_id++) {
		for (rank_id = 0; rank_id < MAX_RANK_COUNT; rank_id++) {
			memset((void *) &rank[rank_id].r_p_info, 0, sizeof(PowerCounter));
			memset((void *) &rank[rank_id].r_p_gblinfo, 0, sizeof(PowerCounter));
			memset((void *) rank[rank_id].bank, 0, sizeof(Bank)
					* MAX_BANK_COUNT);
			rank[rank_id].cke_bit = false;
			if (config->single_rank) {
				memset((void*) &dimm[rank_id], 0, sizeof(DIMM));
				rank[rank_id].my_dimm = &dimm[rank_id];
			} else {
				if ((rank_id & 0x1)) { // Odd dimm
					rank[rank_id].my_dimm = &dimm[rank_id - 1];
				} else {
					memset((void *) &dimm[rank_id], 0, sizeof(DIMM));
					rank[rank_id].my_dimm = &dimm[rank_id];

				}
			}
		}
		memset((void *) &sched_cmd_info, 0, sizeof(RBRR));
		sched_cmd_info.current_cmd = RAS;
		sched_cmd_info.last_cmd = MEM_STATE_INVALID;
		sched_cmd_info.last_ref = false;

	}
}

int Memory_Controller::row_command_bus_idle() {

	if ((command_bus.status == IDLE)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int Memory_Controller::col_command_bus_idle() {

	if (command_bus.status == IDLE) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int Memory_Controller::is_cmd_bus_idle(Command *this_c) {
	if (this_c->command == CAS || this_c->command == CAS_WRITE
			|| this_c->command == CAS_WRITE_AND_PRECHARGE || this_c->command
			== CAS_WITH_DRIVE || this_c->command == CAS_AND_PRECHARGE) {
		return col_command_bus_idle();
	} else {
		return row_command_bus_idle();
	}
}

int Memory_Controller::up_bus_idle() {

	if ((config->dram_type == FBD_DDR2) && (up_bus.status == IDLE)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int Memory_Controller::down_bus_idle() {

	if ((config->dram_type == FBD_DDR2) && (down_bus.status == IDLE)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void Memory_Controller::set_row_command_bus(int command, int current_dram_time) {

	command_bus.status = BUS_BUSY;
	command_bus.command = command;
	command_bus.completion_time = current_dram_time
			+ config->col_command_duration;
}

void Memory_Controller::set_col_command_bus(int command, int current_dram_time) {

	command_bus.status = BUS_BUSY;
	command_bus.command = command;
	command_bus.completion_time = current_dram_time
			+ config->col_command_duration;
	// Hack for posted CAS systems needed
	if (config->posted_cas_flag && (command == CAS || command == CAS_WRITE
			|| command == CAS_AND_PRECHARGE || command
			== CAS_WRITE_AND_PRECHARGE))
		command_bus.completion_time += config->row_command_duration;
}

bool Memory_Controller::can_issue_command(int tran_index, Command *this_c,
		int time) {
	switch (this_c->command) {
	case CAS:
	case CAS_WITH_DRIVE:
	case CAS_WRITE:
		return can_issue_cas(tran_index, this_c, time);
	case CAS_WRITE_AND_PRECHARGE:
	case CAS_AND_PRECHARGE:
		return can_issue_cas_with_precharge(tran_index, this_c, time);
	case RAS:
		return can_issue_ras(tran_index, this_c, time);
	case PRECHARGE:
		return can_issue_prec(tran_index, this_c, time);
		///case DATA:
		///return can_issue_data(tran_index,this_c, time);
		///case DRIVE:
		/// return can_issue_drive(tran_index, this_c, time);
	default:
		fprintf(stdout,"Not sure of command type, unable to issue");
		return false;
	}

	return false;
}

/*
 * This code checks to see if the bank is open, or will be open in time, and if the data bus is free so a CAS can be issued.
 * command is read or write
 */

bool Memory_Controller::can_issue_cas(int tran_index, Command *this_c,
		int current_dram_time) {
	int i;
	int bank_is_open;
	int bank_willbe_open;
	int rank_id, bank_id, row_id;
	tick_t data_bus_free_time = 0;
	tick_t data_bus_reqd_time = 0;

	rank_id = this_c->rank_id;
	bank_id = this_c->bank_id;
	row_id = this_c->row_id;

	bank_is_open = FALSE;
	bank_willbe_open = FALSE;

	data_bus_free_time = data_bus.completion_time;

	if (config->dram_type == FBD_DDR2) {
		// return (fbd_can_issue_cas(tran_index, this_c));

	} else {
		if (this_c->command == CAS || this_c->command == CAS_AND_PRECHARGE) {
			data_bus_reqd_time = current_dram_time + config->t_cas;
			if (data_bus.current_rank_id != rank_id) { /* if the existing data bus isn't owned by the current rank */
				data_bus_free_time += config->t_dqs;
			}
			/* In these systems, a read cannot start in a given rank until writes fully complete */
			if ((config->dram_type == SDRAM) || (config->dram_type == DDRSDRAM)
					|| (config->dram_type == DDR2)) {
				if (rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time
						> (current_dram_time + config->t_cmd)) {
#ifdef DEBUG
					if (config->get_tran_watch(this_c->tran_id)) {
						fprintf(
								stdout,"[%llu]CAS(%llu) t_wr conflict  [%llu] \n",
								current_dram_time,
								this_c->tran_id,
								rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time);
					}
#endif
					return false;

				}
			}
		} else if (this_c->command == CAS_WRITE || this_c->command
				== CAS_WRITE_AND_PRECHARGE) {
			if (data_bus.current_rank_id != MEM_STATE_INVALID) { /* if the existing data bus isn't owned by the controller */
				data_bus_free_time += config->t_dqs;
			}
			if (config->get_tran_watch(this_c->tran_id)) {
				fprintf(
						stdout," [%llu] Seting data bus reqd time to %llu + t_cwd %d\n",
						current_dram_time, current_dram_time, config->t_cwd);
			}
			if (config->t_cwd >= 0)
				data_bus_reqd_time = current_dram_time + config->t_cwd;
			else
				data_bus_reqd_time = current_dram_time;
		}

		if (config->posted_cas_flag == TRUE) {
			data_bus_reqd_time += config->t_al; // This seems to happen ??
		}

		if (data_bus_free_time > data_bus_reqd_time) { /* if data bus isn't going to be free in time, return false */
#ifdef DEBUG
			if (config->get_tran_watch(this_c->tran_id)) {
				fprintf(
						stdout,"[%llu]CAS(%llu) Data Bus Not Free  till [%llu] reqd [%llu]\n",
						current_dram_time, this_c->tran_id, data_bus_free_time,
						data_bus_reqd_time);
			}
#endif
			return false;
		}

		if (config->row_buffer_management_policy == PERFECT_PAGE) { /* This policy doesn't case about issue rules as long as data bus is ready */
			return true;
		}

		if (config->strict_ordering_flag == TRUE) { /* if strict_ordering is set, suppress if earlier CAS exists in queue */
			for (i = 0; i < tran_index; i++) {
				if (!transaction_queue->entry[i].issued_cas) {
#ifdef DEBUG
					if (config->get_tran_watch(this_c->tran_id)) {
						fprintf(
								stdout,"[%llu]CAS(%llu) Strict Ordering In force \n",
								current_dram_time, this_c->tran_id);
					}
#endif
					return false;
				}
			}
		}
		/** Check if all previous Refresh commands i.e. RAS_ALL /PREC_ALL have been
		 * issued **/
		if (config->auto_refresh_enabled == TRUE) {
			if (check_for_refresh(this_c, tran_index, current_dram_time,
					current_dram_time) == FALSE) {
#ifdef DEBUG
				if (config->get_tran_watch(this_c->tran_id)) {
					fprintf(stdout,"[%llu]CAS(%llu) Refresh conflict  \n",
							current_dram_time, this_c->tran_id);
				}
#endif
				return false;
			}
		}
		/* Check if bank is ready i.e.
		 * rcd_done_time > rp_done_time
		 * row open == reqd row
		 * rcd_done_time <= dram_reqd_time
		 */
		tick_t dram_reqd_time = current_dram_time
				+ config->col_command_duration;
		if (config->posted_cas_flag == TRUE)
			dram_reqd_time += config->t_al;
		if ((rank[rank_id].bank[bank_id].rcd_done_time
				> rank[rank_id].bank[bank_id].rp_done_time)
				&& (rank[rank_id].bank[bank_id].row_id == row_id)
				&& (rank[rank_id].bank[bank_id].rcd_done_time <= dram_reqd_time)) {
#ifdef DEBUG
			if (config->get_tran_watch(this_c->tran_id)) {
				fprintf(stdout," Issuing the CAS\n");
			}
#endif
			return true;
		} else {
#ifdef DEBUG
			if (config->get_tran_watch(this_c->tran_id)) {
				fprintf(
						stdout,"[%llu]CAS(%llu) Bank Not Open: rcd_done [%llu] rp_done[%llu] ras_done[%llu] Status (%d) row_id(%x) my row_id (%x)\n",
						current_dram_time, this_c->tran_id,
						rank[rank_id].bank[bank_id].rcd_done_time,
						rank[rank_id].bank[bank_id].rp_done_time,
						rank[rank_id].bank[bank_id].ras_done_time,
						rank[rank_id].bank[bank_id].status,
						rank[rank_id].bank[bank_id].row_id, row_id);
			}
#endif
			return false;

		}
	}
#ifdef DEBUG
	if (config->get_tran_watch(this_c->tran_id)) {
		fprintf(stdout," Issuing the CAS\n");
	}
#endif
	return true;
}

// Just calculates the cmd bus and dram reqd time
void Memory_Controller::get_reqd_times(Command * this_c,
		tick_t *cmd_bus_reqd_time, tick_t *dram_reqd_time,
		int current_dram_time) {
	if (config->dram_type == FBD_DDR2) {
		*cmd_bus_reqd_time = current_dram_time
				+ (config->var_latency_flag ? config->t_bus * (this_c->rank_id)
						: config->t_bus * (config->rank_count - 1))
				+ config->t_bundle + config->t_amb_up;
	} else
		*cmd_bus_reqd_time = current_dram_time;

	*dram_reqd_time = *cmd_bus_reqd_time + config->row_command_duration;
	return;
}

/*
 *  RAS can only be issued if it's idle or if its precharging and trp will expire.
 */

bool Memory_Controller::can_issue_ras(int tran_index, Command *this_c,
		int current_dram_time) {

	int rank_id = this_c->rank_id, bank_id = this_c->bank_id;
	tick_t cmd_bus_reqd_time = 0;
	tick_t dram_reqd_time = 0;
	Transaction * this_t = &(transaction_queue->entry[tran_index]);
	Command *temp_c = NULL;
	int i;
	get_reqd_times(this_c, &cmd_bus_reqd_time, &dram_reqd_time,
			current_dram_time);

	/* Check that your dat has been issued for a write command */
	if (config->dram_type == FBD_DDR2 && this_t->transaction_type
			== MEMORY_WRITE_COMMAND && !this_t->issued_data) {
#ifdef DEBUG
		if (config->get_tran_watch(this_c->tran_id))
			fprintf(
					stdout,"[%llu]RAS(%llu) Data still not started to be issued ",
					current_dram_time, this_c->tran_id);
#endif
		return false;
	}
	/* Check for conflicts with refresh */
	if (config->auto_refresh_enabled == TRUE) {
		if (check_for_refresh(this_c, tran_index, cmd_bus_reqd_time,
				current_dram_time) == FALSE) {
#ifdef DEBUG
			if (config->get_tran_watch(this_c->tran_id))
				fprintf(stdout,"[%llu]RAS(%lu) Refresh Conflict\n",
						current_dram_time, this_c->tran_id);
#endif
			return false;
		}
	}

	/* Check for whether the bank is in the right state */
	if ((rank[rank_id].bank[bank_id].ras_done_time
			<= rank[rank_id].bank[bank_id].rp_done_time)
			&& (rank[rank_id].bank[bank_id].rp_done_time <= dram_reqd_time)) {
	} else {
#ifdef DEBUG
		if (config->get_tran_watch(this_c->tran_id))
			fprintf(
					stdout,"[%llu]RAS(%llu) Bank Conflict RP Done Time %llu Ras done time %llu \n",
					current_dram_time, this_c->tran_id,
					rank[rank_id].bank[bank_id].rp_done_time,
					rank[rank_id].bank[bank_id].ras_done_time);
#endif
		return false;
	}
	/** Check for conflict on command bus **/
	if (check_cmd_bus_required_time(this_c, cmd_bus_reqd_time) == FALSE) {
#ifdef DEBUG
		if (config->get_tran_watch(this_c->tran_id))
			printf("[%llu]RAS(%llu) DIMM Bus Busy \n", current_dram_time,
					this_c->tran_id);
#endif
		return false;
	}
	/* If type = DDR2 or DDR3, check to see that tRRD and tFAW is honored */
	if ((config->dram_type == DDR2) || (config->dram_type == DDR3)
			|| (config->dram_type == FBD_DDR2)) {
		if ((current_dram_time - rank[this_c->rank_id].activation_history[0])
				< config->t_rrd) {
			t_rrd_violated++;
#ifdef DEBUG
			if (config->get_tran_watch(this_c->tran_id)) {
				fprintf(stdout,"[%llu]RAS(%llu) t_RRD violated \n",
						current_dram_time, this_c->tran_id);
			}
#endif
			return false; /* violates t_RRD */
		} else if ((current_dram_time
				- rank[this_c->rank_id].activation_history[3]) < config->t_faw) {
			/*
			 fprintf(stdout,"tFAW [%d] \n",(int)dram_controller[this_c->chan_id].rank[this_c->rank_id].activation_history[3]);
			 */
			t_faw_violated++;
#ifdef DEBUG
			if (config->get_tran_watch(this_c->tran_id)) {
				fprintf(stdout,"[%llu]RAS(%llu) t_FAW violated \n",
						current_dram_time, this_c->tran_id);
			}
#endif
			return false; /* violates t_FAW */
		}
	}

	/*** We have to traverse the entire queue unless we set up a data bus
	 * for each rank and track it
	 * Do not issue ras for the followign reasons
	 * 1. Previous commands i.e. RAS/CAS to the same bank are incomplete i.e.
	 * reached mem_state_complete.
	 * **/
	int sched_policy = config->get_transaction_selection_policy();
	for (i = 0; i < transaction_queue->transaction_count; i++) {
		temp_c = transaction_queue->entry[i].next_c;
		if (temp_c->rank_id == rank_id && temp_c->bank_id == bank_id) {
			if (temp_c->row_id == this_c->row_id && i < tran_index
					&& !transaction_queue->entry[i].status != MEM_STATE_ISSUED) {
				/* this implies that its probably this trans that issued the
				 * precharge */
#ifdef DEBUG
				if (config->get_tran_watch(this_c->tran_id))
					fprintf(
							stdout,"[%llu]RAS(%llu) !issued cmd Tran[%d] Issued command-staus[%d]  \n",
							current_dram_time, this_c->tran_id, i,
							transaction_queue->entry[i].status);
#endif
				return false;
			}
			if (config->row_buffer_management_policy == OPEN_PAGE && i
					!= tran_index && transaction_queue->entry[i].status
					!= MEM_STATE_SCHEDULED) {
				if (this_t->status != MEM_STATE_ISSUED) {
					if (transaction_queue->entry[i].status == MEM_STATE_ISSUED) {
						/* this implies that its probably this trans that issued the
						 * precharge */
#ifdef DEBUG
						if (config->get_tran_watch(this_c->tran_id))
							fprintf(
									stdout,"[%llu]RAS(%llu) !issued cmd Tran[%d] Issued command-staus[%d]  \n",
									current_dram_time, this_c->tran_id, i,
									transaction_queue->entry[i].status);
#endif
						return false;
					}
				}
			} else if (config->row_buffer_management_policy == CLOSE_PAGE && i
					< tran_index && transaction_queue->entry[i].status
					!= MEM_STATE_SCHEDULED) {

			}
			if (sched_policy == FCFS && i < tran_index && temp_c->status
					== IN_QUEUE && temp_c->command != DATA) { // you should never go out of order
#ifdef DEBUG
				if (config->get_tran_watch(this_c->tran_id))
					fprintf(
							stdout,"[%lu]RAS(%lu) Bank open to row reqd by [%lu] \n",
							current_dram_time, this_c->tran_id, temp_c->tran_id);
#endif
				return false;
			}

		}

	}

	/** Should we be doing this ?? **/
	if (config->posted_cas_flag == TRUE && this_c->refresh == FALSE) { /* posted CAS flag set */
		if ((config->dram_type == DDR2) || (config->dram_type == DDR3)) {
			tick_t data_bus_free_time = data_bus.completion_time;
			tick_t data_bus_reqd_time = 0;
			if (this_t->transaction_type == MEMORY_WRITE_COMMAND) {
				if (data_bus.current_rank_id != MEM_STATE_INVALID) { /* if the existing data bus isn't owned by the current rank */
					data_bus_free_time += config->t_dqs;
				}
				data_bus_reqd_time = dram_reqd_time
						+ config->col_command_duration + config->t_al
						+ config->t_cwd;
			} else {
				if (data_bus.current_rank_id != rank_id) { /* if the existing data bus isn't owned by the current rank */
					data_bus_free_time += config->t_dqs;
				}
				data_bus_reqd_time = dram_reqd_time + config->t_cas
						+ config->t_al;
			}
			if (data_bus_free_time > data_bus_reqd_time) { /* if data bus isn't going to be free in time, return false */
#ifdef DEBUG
				if (config->get_tran_watch(this_c->tran_id)) {
					fprintf(
							stdout,"[%lu]RAS -> posted CAS(%lu) Data Bus Conflict: Bus not free \n",
							current_dram_time, this_c->tran_id);
				}
#endif
				return false;
			}
		}/**else if (fbd_can_issue_ras_with_posted_cas(tran_index,this_c->next_c) == false ) {
		 #ifdef DEBUG
		 if (config->get_tran_watch(this_c->tran_id)) {
		 fprintf(stdout,"[%lu]RAS-> posted CAS failed %llu\n",current_dram_time, this_c->tran_id);
		 }
		 #endif
		 return false;
		 }**/
	}
	return true;
}

/*
 *	We can issue a precharge as long as the bank is active, no pending CAS is waiting to use it, AND t_ras has expired.
 */

bool Memory_Controller::can_issue_prec(int tran_index, Command *this_c,
		int current_dram_time) {
	int j;
	Command *temp_c = NULL;

	Transaction * this_t = &(transaction_queue->entry[tran_index]);
	tick_t cmd_bus_reqd_time, dram_reqd_time;
	get_reqd_times(this_c, &cmd_bus_reqd_time, &dram_reqd_time,
			current_dram_time);
	int sched_policy = config->get_transaction_selection_policy();

	// Check command bus availibility
	if (check_cmd_bus_required_time(this_c, cmd_bus_reqd_time) == FALSE) {
#ifdef DEBUG
		if (config->get_tran_watch(this_c->tran_id))
			fprintf(stdout,"[%llu]PREC(%llu) Cmd bus not available \n",
					current_dram_time, this_c->tran_id);
#endif
		return false;
	}
	// Check that the dram writes and cas are done on time
	if ((rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time
			> dram_reqd_time)
			|| (rank[this_c->rank_id].bank[this_c->bank_id].cas_done_time
					> dram_reqd_time)) {
#ifdef DEBUG
		if (config->get_tran_watch(this_c->tran_id)) {
			fprintf(
					stdout,"[%llu]CAS(%llu) t_wr conflict  [%llu] || cas_done time conflict [%llu] \n",
					current_dram_time, this_c->tran_id,
					rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time,
					rank[this_c->rank_id].bank[this_c->bank_id].cas_done_time);
		}
#endif
		return false;
	}

	// Check that for open page you dont try to close the page that is yours ..
	if (config->row_buffer_management_policy == OPEN_PAGE
			&& ((rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time
					>= rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time
					&& rank[this_c->rank_id].bank[this_c->bank_id].row_id
							== this_c->row_id)
					|| (rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time
							<= rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time))) {
#ifdef DEBUG
		if (config->get_tran_watch(this_c->tran_id)) {
			fprintf(
					stdout,"PREC [%llu] Bank does not need precharge(%llu) ras done [%llu] rp done[%llu] row_id[%x] my row id[%x]\n",
					current_dram_time, this_c->tran_id,
					rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time,
					rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time,
					rank[this_c->rank_id].bank[this_c->bank_id].row_id,
					this_c->row_id);

		}
#endif
		return false;
	}
	/* Check that your dat has been issued for a write command */
	if (config->dram_type == FBD_DDR2 && this_t->transaction_type
			== MEMORY_WRITE_COMMAND && !this_t->issued_data) {
#ifdef DEBUG
		if (config->get_tran_watch(this_c->tran_id))
			printf("[%llu]PREC(%llu) Data still not started to be issued ",
					current_dram_time, this_c->tran_id);
#endif
		return false;
	}

	/** Check if all previous Refresh commands i.e. RAS_ALL /PREC_ALL have been
	 * issued **/
	if (config->auto_refresh_enabled == TRUE) {
		if (check_for_refresh(this_c, tran_index, cmd_bus_reqd_time,
				current_dram_time) == FALSE) {
#ifdef DEBUG
			if (config->get_tran_watch(this_c->tran_id))
				fprintf(stdout,"[%llu]PREC(%llu) Conflict with Refresh \n",
						current_dram_time, this_c->tran_id);
#endif
			return false;
		}
	}
	/*** Now check for conflicts with your own ras and cas commands */
	if (config->row_buffer_management_policy == CLOSE_PAGE) {
		temp_c = transaction_queue->entry[tran_index].next_c;
		while (temp_c != NULL) {
			if (temp_c->command != PRECHARGE && temp_c->command != DRIVE) { /** Ensure that you dont check with yourself and only with dram commands**/
				if (temp_c->status == IN_QUEUE) {
					return false;
#ifdef DEBUG
					if (config->get_tran_watch(this_c->tran_id)) {
						fprintf(
								stdout,"[%llu]PREC(%llu) Command still in queue ",
								current_dram_time, this_c->tran_id);
						temp_c->print_command(current_dram_time);
					}
#endif
				}
			}
			temp_c = temp_c->next_c;
		}
	}

	/** Takes care of RAS conflicts i.e. ensures tRAS is done **/
	if (rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time
			> dram_reqd_time) {
#ifdef DEBUG
		if (config->get_tran_watch(this_c->tran_id)) {
			fprintf(stdout,"[%llu]PREC (%llu) RAS Done conflict {%lld]\n",
					current_dram_time, this_c->tran_id,
					rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time);
		}
#endif
		return false;
	}

	// Brinda: FIXME : Need to make sure that in fcfs you dont trample on
	// transactions which are going to the same bank
	for (j = 0; j < ((sched_policy == FCFS) ? tran_index
			: transaction_queue->transaction_count); j++) { /* checked all preceeding transactions for not yet completed CAS command */
		temp_c = transaction_queue->entry[j].next_c;

		if ((temp_c->rank_id == this_c->rank_id) && j != tran_index
				&& temp_c->bank_id == this_c->bank_id) {
			// If you are to the same row and this is an earlier transaction dont do
			// anything?
			if (temp_c->row_id == this_c->row_id && j < tran_index
					&& !transaction_queue->entry[j].issued_cas) {
#ifdef DEBUG
				if (config->get_tran_watch(this_c->tran_id))
					fprintf(
							stdout,"[%llu]PREC (%llu) Conflict with earlier transaction to same row [%llu]  \n",
							current_dram_time, this_c->tran_id, temp_c->tran_id);
#endif
				return false;
			}

			if (config->row_buffer_management_policy == OPEN_PAGE && j
					!= tran_index) {
				if (temp_c->row_id
						== rank[this_c->rank_id].bank[this_c->bank_id].row_id) {
					// The bank is open to the correct row
					// If the transaction is a read or has data issued then let it go
					if (transaction_queue->entry[j].status
							!= MEM_STATE_SCHEDULED
							&& !transaction_queue->entry[j].issued_cas
							&& transaction_queue->entry[j].status
									!= MEM_STATE_COMPLETED) {
#ifdef DEBUG
						if (config->get_tran_watch(this_c->tran_id))
							fprintf(
									stdout,"[%llu]PREC (%llu) Bank open to row reqd by [%llu] %d \n",
									current_dram_time, this_c->tran_id,
									temp_c->tran_id,
									transaction_queue->entry[j].status);
#endif
						return false;
					}

				} else { // bank is open to Different row =>
					if (transaction_queue->entry[j].status == MEM_STATE_ISSUED) {
#ifdef DEBUG
						if (config->get_tran_watch(this_c->tran_id))
							fprintf(
									stdout,"[%llu]PREC (%llu) Same row[%llu] waiting for it to get done\n",
									current_dram_time, this_c->tran_id,
									temp_c->tran_id);
#endif
						return false;
					}
				}
				if (sched_policy == FCFS) { // you should never go out of order
#ifdef DEBUG
					if (config->get_tran_watch(this_c->tran_id))
						fprintf(
								stdout,"[%llu]PREC (%llu) Bank open to row reqd by [%llu] \n",
								current_dram_time, this_c->tran_id,
								temp_c->tran_id);
#endif
					return false;
				}

			} else { // Close page

				if (transaction_queue->entry[j].status == MEM_STATE_ISSUED && j
						!= tran_index) {
					while (temp_c != NULL) {
						if (temp_c->command != DATA && temp_c->status
								== IN_QUEUE) {
#ifdef DEBUG
							if (config->get_tran_watch(this_c->tran_id)) {
								printf(
										"[%llu]PREC(%llu) Trans status (%d) Conflict with command ",
										current_dram_time, this_c->tran_id,
										transaction_queue->entry[j].status);
								temp_c->print_command(current_dram_time);
							}
#endif
							return false;
						}
						temp_c = temp_c->next_c;
					}
				}
			}
		}
	}

	return true;

}

bool Memory_Controller::can_issue_cas_with_precharge(int tran_index,
		Command *this_c, int current_dram_time) {
	int retval = FALSE;
	if ((retval = can_issue_cas(tran_index, this_c, current_dram_time)) == TRUE) {
		int j;
		Command *temp_c = NULL;

		int sched_policy = config->get_transaction_selection_policy();

		tick_t dram_reqd_time = current_dram_time + config->t_cas
				+ config->t_burst;
		if (config->dram_type == FBD_DDR2) {
			dram_reqd_time += (config->var_latency_flag ? config->t_bus
					* (this_c->rank_id) : config->t_bus * (config->rank_count
					- 1)) + config->t_bundle + config->t_amb_up;
		}
		/*** Your RAS should be issued and all your data should have been sent*/
		if (config->row_buffer_management_policy == CLOSE_PAGE) {
			if (transaction_queue->entry[tran_index].status != MEM_STATE_ISSUED) {
#ifdef DEBUG
				if (config->get_tran_watch(this_c->tran_id)) {
					fprintf(stdout,"[%llu]PREC(%llu) Transaction not started ",
							current_dram_time, this_c->tran_id);
				}
#endif
				return false;
			}
		}

		/** Takes care of RAS conflicts i.e. ensures tRAS is done **/
		if (rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time
				> dram_reqd_time) {
#ifdef DEBUG
			if (config->get_tran_watch(this_c->tran_id)) {
				fprintf(
						stdout,"[%llu]PREC (%llu) RAS Done conflict [%lld] > [%llu]\n",
						current_dram_time,
						this_c->tran_id,
						rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time,
						dram_reqd_time);
			}
#endif
			return false;
		}

		// Brinda: FIXME : Need to make sure that in fcfs you dont trample on
		// transactions which are going to the same bank
		for (j = 0; j < ((sched_policy == FCFS) ? tran_index
				: transaction_queue->transaction_count); j++) { /* checked all preceeding transactions for not yet completed CAS command */
			temp_c = transaction_queue->entry[j].next_c;
			if ((temp_c->rank_id == this_c->rank_id) && j != tran_index
					&& temp_c->bank_id == this_c->bank_id) {
				if ((temp_c->rank_id == this_c->rank_id) && j != tran_index
						&& temp_c->bank_id == this_c->bank_id) {
					// If you are to the same row and this is an earlier transaction dont do
					// anything?
					if (temp_c->row_id == this_c->row_id && j < tran_index
							&& !transaction_queue->entry[j].issued_cas) {
#ifdef DEBUG
						if (config->get_tran_watch(this_c->tran_id))
							fprintf(
									stdout,"[%llu]CAS_WITH_PREC (%llu) Conflict with earlier transaction to same row [%llu]  \n",
									current_dram_time, this_c->tran_id,
									temp_c->tran_id);
#endif
						return false;
					}
				}
				if (sched_policy == FCFS
						&& !transaction_queue->entry[j].issued_cas) { // you should never go out of order
#ifdef DEBUG
					if (config->get_tran_watch(this_c->tran_id))
						fprintf(
								stdout,"[%llu]CAS_WITH_PREC (%llu) Bank open to row reqd by [%llu] \n",
								current_dram_time, this_c->tran_id,
								temp_c->tran_id);
#endif
					return false;
				}
				if (config->row_buffer_management_policy == CLOSE_PAGE) {
					if (transaction_queue->entry[j].status == MEM_STATE_ISSUED) {
						while (temp_c != NULL) {
							if (temp_c->command != DATA && temp_c->status
									== IN_QUEUE) {
#ifdef DEBUG
								if (config->get_tran_watch(this_c->tran_id)) {
									printf(
											"[%llu]CAS_WITH_PREC(%llu) Trans status (%d) Conflict with command ",
											current_dram_time, this_c->tran_id,
											transaction_queue->entry[j].status);
									temp_c->print_command(current_dram_time);
								}
#endif
								return false;
							}
							temp_c = temp_c->next_c;
						}
					}
				}
			}
		}
		return true;
	} else {
		return false;
	}
}

/***
 * 1. If issued -> Refresh has to wait for the transaction to complete -> note that this could be
 * incorrect behavior -> but am not sure of how the memory system actually
 * handles this case unless refresh issue policy is opportunistic
 * 2. If not issued -> Check if refresh is going to be done in time to issue
 * i.e.  refresh has completed.
 * 3. Check if there is a command bus conflict - laready taken care of earlier
 */
bool Memory_Controller::check_for_refresh(Command * this_c, int tran_index,
		tick_t cmd_bus_reqd_time, int current_dram_time) {
	Command *temp_c = NULL, *rq_temp_c = NULL;

	rq_temp_c = refresh_queue.rq_head;
	if (this_c->refresh == FALSE) {
		if (transaction_queue->entry[tran_index].status != MEM_STATE_ISSUED) {
			if ((config->refresh_issue_policy == REFRESH_OPPORTUNISTIC
					&& refresh_queue.is_refresh_queue_half_full())
					|| (config->refresh_issue_policy
							== REFRESH_HIGHEST_PRIORITY)) {
				while (rq_temp_c != NULL) {
					temp_c = rq_temp_c;
					/** Only check for unissued Refresh stuff **/
					while (temp_c != NULL) {
						if (temp_c->status == IN_QUEUE) {
							if (config->refresh_policy
									== REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK
									|| (config->refresh_policy
											== REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK
											&& temp_c->bank_id
													== this_c->bank_id)
									|| (config->refresh_policy
											== REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK
											&& temp_c->rank_id
													== this_c->rank_id)
									|| (config->refresh_policy
											== REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK
											&& temp_c->rank_id
													== this_c->rank_id
											&& temp_c->bank_id
													== this_c->bank_id)) {
#ifdef DEBUG
								if (config->get_tran_watch(this_c->tran_id)) {
									fprintf(stdout, "----------------------\n");
									fprintf(
											stdout,"[%llu]CMD (%d) (%llu) Refresh Conflict tstatus(%d) tindex(%d) ",
											current_dram_time,
											this_c->command,
											this_c->tran_id,
											transaction_queue->entry[tran_index].status,
											tran_index);
									temp_c->print_command(current_dram_time);
									fprintf(stdout,"----------------------\n");
								}
#endif
								return false;
							}
						}
						temp_c = temp_c->next_c;
					}
					rq_temp_c = rq_temp_c->rq_next_c;
				}
			}
		}
	}
	return true;
}

bool Memory_Controller::check_data_bus_required_time(Command * this_c,
		tick_t data_bus_reqd_time) {

	if (data_bus_reqd_time
			< rank[this_c->rank_id].my_dimm->data_bus_completion_time)
		return false;

	return true;
}
/**
 * Function that checks if the command to be issued conflicts with a previously
 * issued command. Returns FALSE if there is a conflict else returns TRUE.
 *
 * Command this_c - Command which is to be issued.
 * Command temp_c - Command which has been already been issued
 * tick_t cmd_bus_reqd_time  Time at which command bus is required
 * */

bool Memory_Controller::check_cmd_bus_required_time(Command * this_c,
		tick_t cmd_bus_reqd_time) {
	/* We just need to worry abotu transmission time */
	if (config->dram_type == FBD_DDR2) {
		if (cmd_bus_reqd_time
				< rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time)
			return false;
	} else {
		return (col_command_bus_idle());
	}
	return true;
}

bool Memory_Controller::check_down_bus_required_time(Command * this_c,
		Command *temp_c, tick_t down_bus_reqd_time) {
	if (config->var_latency_flag == TRUE) {
		if (this_c->rank_id > temp_c->rank_id) {
			if (down_bus_reqd_time < (temp_c->link_data_tran_comp_time
					- (this_c->rank_id - temp_c->rank_id) * config->t_bus))
				return false;
		} else {
			if (down_bus_reqd_time < (temp_c->link_data_tran_comp_time
					- (temp_c->rank_id - this_c->rank_id) * config->t_bus))
				return false;
		}
	} else {

		if (down_bus_reqd_time < (temp_c->link_data_tran_comp_time
				- (config->rank_count) * config->t_bus))
			return false;
	}
	return true;
}

void Memory_Controller::update_refresh_status(tick_t now, char * debug_string) {
	/* Traverse through queue and update the status of every command **/
	Command *temp_rq_c;
	Command *prev_rq_c;
	Command *temp_c;
	temp_rq_c = refresh_queue.rq_head;
	prev_rq_c = NULL;
	int done, remove_refresh_tran = FALSE;
	while (temp_rq_c != NULL) {
		temp_c = temp_rq_c;
		done = TRUE;
		while (temp_c != NULL) {
			switch (temp_c->command) {
			case PRECHARGE:
				precharge_transition(temp_c, temp_c->tran_id, id,
						temp_c->rank_id, temp_c->bank_id, temp_c->row_id,
						debug_string, now);
				break;
			case PRECHARGE_ALL:
				precharge_all_transition(temp_c, temp_c->tran_id, id,
						temp_c->rank_id, temp_c->bank_id, temp_c->row_id,
						debug_string, now);
				break;
			case REFRESH:
				refresh_transition(temp_c, temp_c->tran_id, id,
						temp_c->rank_id, temp_c->bank_id, temp_c->row_id,
						debug_string, now);
				break;
			case REFRESH_ALL:
				refresh_all_transition(temp_c, temp_c->tran_id, id,
						temp_c->rank_id, temp_c->bank_id, temp_c->row_id,
						debug_string, now);
				break;

			}
			if (temp_c->status != MEM_STATE_COMPLETED)
				done = FALSE;
			temp_c = temp_c->next_c;

		}
		if (done == TRUE) {
			remove_refresh_tran = TRUE;
			remove_refresh_transaction(prev_rq_c, temp_rq_c, now);
			if (prev_rq_c == NULL)
				temp_rq_c = refresh_queue.rq_head;
			else
				temp_rq_c = prev_rq_c->rq_next_c;
		} else {
			prev_rq_c = temp_rq_c;
			temp_rq_c = temp_rq_c->rq_next_c;
		}
	}
	//if ( remove_refresh_tran == TRUE && config->row_buffer_management_policy == OPEN_PAGE)
	//	update_tran_for_refresh(chan_id);

}

/************************************************************
 * update_command_states
 * Updates the state of the inflight commands of a transaction.
 *
 * tran_index - Transaction index ( != transaction id)
 * debug_string
 * **********************************************************/
int Memory_Controller::update_command_states(tick_t now, int chan_id,
		int tran_index, char *debug_string) {
	int done = TRUE;
	int rank_id, bank_id, row_id, col_id;
	Command *this_c;
	int update = FALSE;
	int all_scheduled = TRUE;
	Transaction * this_t = &transaction_queue->entry[tran_index];
	assert (tran_index < MAX_TRANSACTION_QUEUE_DEPTH);

	this_c = this_t->next_c;
	while (this_c != NULL) {
		assert(strlen(debug_string) < MAX_DEBUG_STRING_SIZE); /** Added to take care of a need to allow larger debug strings **/
		if (this_c->status != IN_QUEUE && this_c->status != MEM_STATE_COMPLETED) { // Only check for those commands which have already been issued and not done
			update = TRUE;
			if (config->cas_wave_debug() || config->wave_debug()) {
				rank_id = this_c->rank_id;
				bank_id = this_c->bank_id;
				row_id = this_c->row_id;
				col_id = this_c->col_id;
				switch (this_c->command) {
				case CAS:
					cas_transition(this_c, tran_index, chan_id, rank_id,
							bank_id, row_id, debug_string, now);
					break;
				case CAS_AND_PRECHARGE:
				case CAS_WITH_DRIVE:
					if (config->dram_type == FBD_DDR2) {
						///fbd_cas_with_drive_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
					} else
						cas_transition(this_c, tran_index, chan_id, rank_id,
								bank_id, row_id, debug_string, now);
					break;
				case CAS_WRITE_AND_PRECHARGE:
				case CAS_WRITE:
					cas_write_transition(this_c, tran_index, chan_id, rank_id,
							bank_id, row_id, debug_string, now);
					break;
				case RAS:
					ras_transition(this_c, tran_index, chan_id, rank_id,
							bank_id, row_id, debug_string, now);
					break;
				case PRECHARGE:
					precharge_transition(this_c, tran_index, chan_id, rank_id,
							bank_id, row_id, debug_string, now);
					break;
				case PRECHARGE_ALL:
					precharge_all_transition(this_c, tran_index, chan_id,
							rank_id, bank_id, row_id, debug_string, now);
					break;
				case RAS_ALL:
					ras_all_transition(this_c, tran_index, chan_id, rank_id,
							bank_id, row_id, debug_string, now);
					break;
				case DRIVE:
					///drive_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
					break;
				case DATA:
					data_transition(this_c, tran_index, chan_id, rank_id,
							bank_id, row_id, debug_string, now);
					break;
				default:
					fprintf(stdout, "I am confused, unknown command type: %d",
							this_c->command);
					break;
				}
				//if (this_c->status == MEM_STATE_COMPLETED)
				// fprintf(stdout,"[%llu] done CMD %d tid %llu completion time %llu status %d \n", now,this_c->command,this_c->tran_id,
				//   this_c->completion_time,
				//   this_c->status);
			} else {
				if (this_c->completion_time <= now) {
					this_c->status = MEM_STATE_COMPLETED;
					// fprintf(stdout,"[%llu] done CMD %d tid %llu completion time %llu status %d \n", now,this_c->command,this_c->tran_id,
					//   this_c->completion_time,
					//   this_c->status);
				}
				// Check about releasing the buffers here
				///if (config->dram_type == FBD_DDR2)
				///check_buffer_release(now,this_c);
			}
		} else if (this_c->status == IN_QUEUE) {
			all_scheduled = FALSE;
		}
		if (this_c->status != MEM_STATE_COMPLETED) {
			done = FALSE;
		}
		this_c = this_c->next_c;

	}
	if (this_t->critical_word_ready_time <= now && this_t->issued_cas) {
		this_t->critical_word_ready = true;
	}
	return done;
}

void Memory_Controller::cas_transition(Command *this_c, int tran_index,
		int chan_id, int rank_id, int bank_id, int row_id, char *debug_string,
		int current_dram_time) {

	if (config->dram_type == FBD_DDR2) {
		///fbd_cas_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
	} else {
		if ((this_c->status == LINK_COMMAND_TRANSMISSION)
				&& (this_c->link_comm_tran_comp_time <= current_dram_time)) {
			command_bus.status = IDLE;
			col_command_bus.status = IDLE;
			this_c->status = DRAM_PROCESSING;
			if ((rank[rank_id].bank[bank_id].last_command == CAS_WRITE)
					|| (rank[rank_id].bank[bank_id].last_command == CAS)
					|| (rank[rank_id].bank[bank_id].last_command
							== CAS_WITH_DRIVE)) {
				aux_stat->mem_gather_stat(GATHER_BANK_HIT_STAT,
				(int) (this_c->dram_proc_comp_time
						- rank[rank_id].bank[bank_id].cas_done_time));
			}
			rank[rank_id].bank[bank_id].cas_done_time
					= this_c->dram_proc_comp_time;
#ifdef DEBUG
			if (config->wave_debug() || config->cas_wave_debug()) {
				build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
			}
#endif
		} else if ((this_c->status == DRAM_PROCESSING)
				&& (this_c->dram_proc_comp_time <= current_dram_time)) {
			this_c->status = LINK_DATA_TRANSMISSION;
			data_bus.status = BUS_BUSY;
#ifdef DEBUG
			if (config->wave_debug() || config->cas_wave_debug())
				build_wave(&debug_string[0], tran_index, this_c, PROC2DATA);
#endif
		} else if ((this_c->status == LINK_DATA_TRANSMISSION)
				&& (this_c->link_data_tran_comp_time <= current_dram_time)) {
			//rank[rank_id].bank[bank_id].cas_count_since_ras++;
			rank[rank_id].bank[bank_id].last_command = CAS;
			data_bus.status = IDLE;
			if (this_c->command == CAS_AND_PRECHARGE) {
				this_c->status = DIMM_PRECHARGING;
#ifdef DEBUG
				if (config->wave_debug() || config->cas_wave_debug())
					build_wave(&debug_string[0], tran_index, this_c,
							LINKDATA2PREC);
#endif
			} else {
				this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
				if (config->wave_debug() || config->cas_wave_debug())
					build_wave(&debug_string[0], tran_index, this_c,
							MEM_STATE_COMPLETED);
#endif
			}
		} else if ((this_c->status == DIMM_PRECHARGING)
				&& (this_c->rank_done_time <= current_dram_time)) {
			this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
			if (config->wave_debug() || config->cas_wave_debug())
				build_wave(&debug_string[0], tran_index, this_c,
						MEM_STATE_COMPLETED);
#endif
		}
	}
}

void Memory_Controller::cas_write_transition(Command *this_c, int tran_index,
		int chan_id, int rank_id, int bank_id, int row_id, char *debug_string,
		int current_dram_time) {

	if (config->dram_type == FBD_DDR2) {
		///fbd_cas_write_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
	}

	else {
		if ((this_c->status == LINK_COMMAND_TRANSMISSION)
				&& (this_c->link_comm_tran_comp_time <= current_dram_time)) {
			command_bus.status = IDLE;
			col_command_bus.status = IDLE;

			if (config->t_cwd == 0) { /* DDR SDRAM type */
				data_bus.status = BUS_BUSY;
				//		data_bus.current_rank_id = MEM_STATE_INVALID;  /* memory controller was "owner of DQS */
				this_c->status = LINK_DATA_TRANSMISSION;
				rank[rank_id].bank[bank_id].cas_done_time = current_dram_time
						+ config->t_burst;
			} else if (config->t_cwd > 0) { /* we should enter into a processing state -RDRAM/DDR II/FCRAM/RLDRAM etc */
				this_c->status = DRAM_PROCESSING;
			} else { /* SDRAM, already transmitting data */
				this_c->status = LINK_DATA_TRANSMISSION;
				//		data_bus.current_rank_id = MEM_STATE_INVALID;  /* memory controller was "owner of DQS */
			}
			if ((rank[rank_id].bank[bank_id].last_command == CAS_WRITE)
					|| (rank[rank_id].bank[bank_id].last_command == CAS)
					|| (rank[rank_id].bank[bank_id].last_command
							== CAS_WITH_DRIVE)) {
				aux_stat->mem_gather_stat(GATHER_BANK_HIT_STAT,
				(int) (this_c->dram_proc_comp_time
						- rank[rank_id].bank[bank_id].cas_done_time));
			}
			rank[rank_id].bank[bank_id].cas_done_time
					= this_c->dram_proc_comp_time;
#ifdef DEBUG
			if (config->wave_debug() || config->cas_wave_debug())
				build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
		} else if ((this_c->status == DRAM_PROCESSING)
				&& (this_c->dram_proc_comp_time <= current_dram_time)) { /* rdram or DDR II */
			data_bus.status = BUS_BUSY;
			this_c->status = LINK_DATA_TRANSMISSION;
			/** FIXME **/
			rank[rank_id].bank[bank_id].cas_done_time = current_dram_time
					+ config->t_burst;
			//	  data_bus.current_rank_id = MEM_STATE_INVALID;  /* memory controller was "owner of DQS */
			if ((rank[rank_id].bank[bank_id].last_command == CAS_WRITE)
					|| (rank[rank_id].bank[bank_id].last_command == CAS)) {
				aux_stat->mem_gather_stat(GATHER_BANK_HIT_STAT,
				(int) (this_c->completion_time
						- rank[rank_id].bank[bank_id].cas_done_time));
			}

#ifdef DEBUG
			if (config->wave_debug() || config->cas_wave_debug())
				build_wave(&debug_string[0], tran_index, this_c, PROC2DATA);
#endif
		} else if ((this_c->status == LINK_DATA_TRANSMISSION)
				&& (this_c->link_data_tran_comp_time <= current_dram_time)) {
			//rank[rank_id].bank[bank_id].cas_count_since_ras++;
			rank[rank_id].bank[bank_id].last_command = CAS_WRITE;
			data_bus.status = IDLE;
			if (this_c->command == CAS_WRITE_AND_PRECHARGE) {
				this_c->status = DIMM_PRECHARGING;
#ifdef DEBUG
				if (config->wave_debug() || config->cas_wave_debug())
					build_wave(&debug_string[0], tran_index, this_c,
							LINKDATA2PREC);
#endif
			} else {
				this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
				if (config->wave_debug() || config->cas_wave_debug())
					build_wave(&debug_string[0], tran_index, this_c,
							MEM_STATE_COMPLETED);
#endif
			}
		} else if ((this_c->status == DIMM_PRECHARGING)
				&& (this_c->rank_done_time <= current_dram_time)) {
			this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
			if (config->wave_debug() || config->cas_wave_debug())
				build_wave(&debug_string[0], tran_index, this_c,
						MEM_STATE_COMPLETED);
#endif
		}
	}

}

void Memory_Controller::ras_transition(Command *this_c, int tran_index,
		int chan_id, int rank_id, int bank_id, int row_id, char *debug_string,
		int current_dram_time) {

	if (config->dram_type == FBD_DDR2) {
		///fbd_ras_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
	} else {

		if ((this_c->status == LINK_COMMAND_TRANSMISSION)
				&& (this_c->link_comm_tran_comp_time <= current_dram_time)) {
			this_c->status = DRAM_PROCESSING;
			rank[rank_id].bank[bank_id].status = ACTIVATING;
			rank[rank_id].bank[bank_id].row_id = this_c->row_id;

			if (rank[rank_id].bank[bank_id].last_command != IDLE) {
				/* DW: Look at this again.
				 * last command set to IDLE if init or REFRESH, then all other RAS due to bank conflict.
				 * Gather stat for bank conflict.
				 */
				aux_stat->mem_gather_stat(GATHER_BANK_CONFLICT_STAT,
				(int) (this_c->completion_time
						- rank[rank_id].bank[bank_id].ras_done_time));
			}

			command_bus.status = IDLE;
			row_command_bus.status = IDLE;
#ifdef DEBUG
			if (config->wave_debug())
				build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
		} else if ((this_c->status == DRAM_PROCESSING)
				&& (this_c->dram_proc_comp_time <= current_dram_time)) {
			rank[rank_id].bank[bank_id].status = ACTIVE;
			rank[rank_id].bank[bank_id].last_command = RAS;
			this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
			if (config->wave_debug())
				build_wave(&debug_string[0], tran_index, this_c,
						MEM_STATE_COMPLETED);
#endif
		}
	}
}

void Memory_Controller::precharge_transition(Command *this_c, int tran_index,
		int chan_id, int rank_id, int bank_id, int row_id, char *debug_string,
		int current_dram_time) {

	/*** For the FB-DIMM PRECHARGE transistions are
	 *
	 * LINK_COMMAND_TRANSMISSION ----> AMB_PROCESSING ----> AMB_COMMAND_TRANs---> DRAM_PROCESSING -----> MEM_STATE_COMPLETED
	 * t(bundle) + t(bus)			t(ambup)  				t(row_command_duration)		t(rp)
	 * ***/

	if (config->dram_type == FBD_DDR2) {
		///fbd_precharge_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);

	}
	if ((this_c->status == LINK_COMMAND_TRANSMISSION)
			&& (this_c->link_comm_tran_comp_time <= current_dram_time)) {
		rank[rank_id].bank[bank_id].status = PRECHARGING;
		//	rank[rank_id].bank[bank_id].rp_done_time = current_dram_time +
		//	  config->t_rp;
		this_c->status = DRAM_PROCESSING;
		command_bus.status = IDLE;
		row_command_bus.status = IDLE;
#ifdef DEBUG
		if (config->wave_debug())
			build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
	} else if ((this_c->status == DRAM_PROCESSING)
			&& (this_c->dram_proc_comp_time <= current_dram_time)) {
		rank[rank_id].bank[bank_id].status = IDLE;
		rank[rank_id].bank[bank_id].last_command = PRECHARGE;
		/*
		 mem_gather_stat(GATHER_CAS_PER_RAS_STAT,
		 rank[rank_id].bank[bank_id].cas_count_since_ras);
		 rank[rank_id].bank[bank_id].cas_count_since_ras = 0;
		 */
		this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
		if (config->wave_debug())
			build_wave(&debug_string[0], tran_index, this_c,
					MEM_STATE_COMPLETED);
#endif
	}
}

void Memory_Controller::precharge_all_transition(Command *this_c,
		int tran_index, int chan_id, int rank_id, int bank_id, int row_id,
		char *debug_string, int current_dram_time) {
	int i, j;
	if (config->dram_type == FBD_DDR2) {
		///fbd_precharge_all_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
	} else {
		if ((this_c->status == LINK_COMMAND_TRANSMISSION)
				&& (this_c->link_comm_tran_comp_time <= current_dram_time)) {
			for (i = 0; i < config->rank_count; i++) {
				for (j = 0; j < config->bank_count; j++) {
					rank[i].bank[j].status = PRECHARGING;
					//		  rank[i].bank[j].rp_done_time =
					//			current_dram_time +
					//			config->t_rp;
				}
			}
			this_c->status = DRAM_PROCESSING;
			command_bus.status = IDLE;
			row_command_bus.status = IDLE;
#ifdef DEBUG
			if (config->wave_debug())
				build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
		} else if ((this_c->status == DRAM_PROCESSING)
				&& (this_c->dram_proc_comp_time <= current_dram_time)) {
			for (i = 0; i < config->rank_count; i++) {
				for (j = 0; j < config->bank_count; j++) {
					rank[i].bank[j].status = IDLE;
					rank[i].bank[j].last_command = PRECHARGE;
					/*
					 mem_gather_stat(GATHER_CAS_PER_RAS_STAT,
					 rank[rank_id].bank[bank_id].cas_count_since_ras);
					 */
					rank[i].bank[j].cas_count_since_ras = 0;
				}
			}
			this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
			if (config->wave_debug())
				build_wave(&debug_string[0], tran_index, this_c,
						MEM_STATE_COMPLETED);
#endif
		}
	}
}

void Memory_Controller::ras_all_transition(Command *this_c, int tran_index,
		int chan_id, int rank_id, int bank_id, int row_id, char *debug_string,
		int current_dram_time) {
	int i, j;
	if (config->dram_type == FBD_DDR2) {
		///fbd_ras_all_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
	} else {
		if ((this_c->status == LINK_COMMAND_TRANSMISSION)
				&& (this_c->link_comm_tran_comp_time <= current_dram_time)) {
			for (i = 0; i < config->rank_count; i++) {
				for (j = 0; j < config->bank_count; j++) {
					rank[i].bank[j].status = ACTIVATING;
					//		  rank[i].bank[j].ras_done_time =
					//			current_dram_time +
					//			config->t_ras;
				}
			}
			this_c->status = DRAM_PROCESSING;
			command_bus.status = IDLE;
			row_command_bus.status = IDLE;
#ifdef DEBUG
			if (config->wave_debug())
				build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
		} else if ((this_c->status == DRAM_PROCESSING)
				&& (this_c->dram_proc_comp_time <= current_dram_time)) {
			for (i = 0; i < config->rank_count; i++) {
				for (j = 0; j < config->bank_count; j++) {
					rank[i].bank[j].status = ACTIVE;
					rank[i].bank[j].last_command = RAS;
					//rank[rank_id].bank[bank_id].cas_count_since_ras = 0;
				}
			}
			this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
			if (config->wave_debug())
				build_wave(&debug_string[0], tran_index, this_c,
						MEM_STATE_COMPLETED);
#endif
		}
	}
}

void Memory_Controller::refresh_all_transition(Command *this_c, int tran_index,
		int chan_id, int rank_id, int bank_id, int row_id, char *debug_string,
		int current_dram_time) {
	int i, j;
	if (config->dram_type == FBD_DDR2) {
		///fbd_refresh_all_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
	} else {
		if ((this_c->status == LINK_COMMAND_TRANSMISSION)
				&& (this_c->link_comm_tran_comp_time <= current_dram_time)) {
			if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
				for (j = 0; j < config->bank_count; j++) {
					rank[rank_id].bank[j].row_id = this_c->row_id;
					rank[rank_id].bank[j].status = ACTIVATING;
					//		  rank[rank_id].bank[j].rp_done_time =
					//			current_dram_time + config->t_rfc;
				}

			} else {
				for (i = 0; i < config->rank_count; i++) {
					if (config->refresh_policy
							== REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
						for (j = 0; j < config->bank_count; j++) {
							rank[i].bank[j].row_id = this_c->row_id;
							rank[i].bank[j].status = ACTIVATING;
							//			  rank[i].bank[j].rp_done_time =
							//				current_dram_time + config->t_rfc;
							//			  rank[i].bank[j].rfc_done_time =
							//				current_dram_time + config->t_rfc;
						}
					} else {
						rank[i].bank[this_c->bank_id].row_id = this_c->row_id;
						rank[i].bank[this_c->bank_id].status = ACTIVATING;
						//			rank[i].bank[this_c->bank_id].rp_done_time =
						//			  current_dram_time + config->t_rfc;
					}
				}
			}
			this_c->status = DRAM_PROCESSING;
			command_bus.status = IDLE;
			row_command_bus.status = IDLE;
#ifdef DEBUG
			if (config->wave_debug())
				build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
		} else if ((this_c->status == DRAM_PROCESSING)
				&& (this_c->dram_proc_comp_time <= current_dram_time)) {
			if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
				for (j = 0; j < config->bank_count; j++) {
					rank[this_c->rank_id].bank[j].status = IDLE;
					rank[this_c->rank_id].bank[j].last_command = REFRESH_ALL;
				}
			} else {
				for (i = 0; i < config->rank_count; i++) {
					if (config->refresh_policy
							== REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
						for (j = 0; j < config->bank_count; j++) {
							rank[i].bank[j].status = IDLE;
							rank[i].bank[j].last_command = REFRESH_ALL;
						}
					} else {
						rank[i].bank[this_c->bank_id].status = IDLE;
						rank[i].bank[this_c->bank_id].last_command
								= REFRESH_ALL;
					}
				}

				//rank[rank_id].bank[bank_id].cas_count_since_ras = 0;
			}
			this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
			if (config->wave_debug())
				build_wave(debug_string, tran_index, this_c,
						MEM_STATE_COMPLETED);
#endif
		}
	}
}
/**
 * send command
 * wait for dram to process = t_ras+t_rp
 */
void Memory_Controller::refresh_transition(Command *this_c, int tran_index,
		int chan_id, int rank_id, int bank_id, int row_id, char *debug_string,
		int current_dram_time) {

	if (config->dram_type == FBD_DDR2) {
		///fbd_refresh_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
	} else {

		if ((this_c->status == LINK_COMMAND_TRANSMISSION)
				&& (this_c->link_comm_tran_comp_time <= current_dram_time)) {
			this_c->status = DRAM_PROCESSING;
			rank[rank_id].bank[bank_id].status = ACTIVATING;
			rank[rank_id].bank[bank_id].row_id = this_c->row_id;
			//	  rank[rank_id].bank[bank_id].rp_done_time =
			//		current_dram_time + config->t_rc;
			//	  rank[rank_id].bank[bank_id].rfc_done_time =
			//		current_dram_time + config->t_rfc;
			command_bus.status = IDLE;
			row_command_bus.status = IDLE;

#ifdef DEBUG
			if (config->wave_debug())
				build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
		} else if ((this_c->status == DRAM_PROCESSING)
				&& (this_c->dram_proc_comp_time <= current_dram_time)) {
			rank[rank_id].bank[bank_id].status = IDLE;
			rank[rank_id].bank[bank_id].last_command = REFRESH;
			this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
			if (config->wave_debug())
				build_wave(&debug_string[0], tran_index, this_c,
						MEM_STATE_COMPLETED);
#endif
		}
	}
}

void Memory_Controller::remove_refresh_transaction(Command *prev_rq_c,
		Command* this_rq_c, int current_dram_time) {
	Command *temp_c;
#ifdef DEBUG
	if (config->tq_info.get_transaction_debug()) {
		fprintf(stdout, "[%llu] Removing Refresh Transaction %llu\n",
				current_dram_time, this_rq_c->tran_id);
	}
#endif
	if (prev_rq_c == NULL) {/* First item in the queue */
		refresh_queue.rq_head = this_rq_c->rq_next_c;
	} else {
		prev_rq_c->rq_next_c = this_rq_c->rq_next_c;
	}
	/** Return the commands to the command pool **/
	temp_c = this_rq_c;
	while (temp_c->next_c != NULL) {
		temp_c = temp_c->next_c;
	}
	temp_c->next_c = config->tq_info.free_command_pool;
	config->tq_info.free_command_pool = this_rq_c;
	this_rq_c->rq_next_c = NULL;
	/** Update the queue size **/
	refresh_queue.size--;

}

void Memory_Controller::update_bus_link_status(int current_dram_time) {
	if (config->dram_type == FBD_DDR2) {
		if (up_bus.completion_time <= current_dram_time) {
			up_bus.status = IDLE;
		}
	} else {
		if (command_bus.completion_time <= current_dram_time) {
			command_bus.status = IDLE;
		}
	}
}

void Memory_Controller::update_refresh_missing_cycles(tick_t now,
		char * debug_string) {
	int i;
	DRAMaddress refresh_address;
	if (now > (config->refresh_cycle_count + refresh_queue.last_refresh_cycle
			+ config->t_rfc)) {
		int num_refresh_missed = (now - refresh_queue.last_refresh_cycle)
				/ config->refresh_cycle_count;
		for (i = 0; i < num_refresh_missed; i++) {
			refresh_address = get_next_refresh_address();
		}

		//refresh_queue.last_refresh_cycle = (now/config->refresh_cycle_count)* config->refresh_cycle_count;
		fprintf(
				stdout,"[%llu] Error: missing refreshes %d Last Refresh [%llu]\n",
				now, num_refresh_missed, refresh_queue.last_refresh_cycle);
		fprintf(
				stdout, "config->refresh_cycle_count = %d, refresh_queue.last_refresh_cycle = %d, config->t_rfc = %d\n",
				config->refresh_cycle_count, refresh_queue.last_refresh_cycle,
				config->t_rfc);
		///print_transaction_queue(now);
		exit(0);
	}
}

/** Function which returns the "refresh address" and updates the channels
 * refresh addresses accordingly **/

DRAMaddress Memory_Controller::get_next_refresh_address() {
	DRAMaddress this_a;
	if (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
		this_a.physical_address = 0;
		this_a.chan_id = id;
		this_a.rank_id = config->rank_count;
		this_a.bank_id = config->bank_count;
		this_a.row_id = refresh_queue.refresh_row_index;
		refresh_queue.refresh_row_index = (refresh_queue.refresh_row_index + 1)
				% config->row_count;
	} else if (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
		this_a.physical_address = 0;
		this_a.chan_id = id;
		this_a.rank_id = config->rank_count;
		this_a.bank_id = refresh_queue.refresh_bank_index;
		refresh_queue.refresh_bank_index = (refresh_queue.refresh_bank_index
				+ 1) % config->bank_count;
		this_a.row_id = refresh_queue.refresh_row_index;
		if (refresh_queue.refresh_bank_index == 0) { /* We rolled over so should increment row count */
			refresh_queue.refresh_row_index = (refresh_queue.refresh_row_index
					+ 1) % config->row_count;
		}
	} else if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK) {
		this_a.physical_address = 0;
		this_a.chan_id = id;
		this_a.rank_id = refresh_queue.refresh_rank_index;
		this_a.bank_id = refresh_queue.refresh_bank_index;
		this_a.row_id = refresh_queue.refresh_row_index;
		refresh_queue.refresh_rank_index = (refresh_queue.refresh_rank_index
				+ 1) % config->rank_count;
		if (refresh_queue.refresh_rank_index == 0) {
			refresh_queue.refresh_bank_index
					= (refresh_queue.refresh_bank_index + 1)
							% config->bank_count;
			if (refresh_queue.refresh_bank_index == 0) { /* We rolled over so should increment row count */
				refresh_queue.refresh_row_index
						= (refresh_queue.refresh_row_index + 1)
								% config->row_count;
			}
		}
	} else if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
		this_a.physical_address = 0;
		this_a.chan_id = id;
		this_a.bank_id = refresh_queue.refresh_bank_index; // Does not matter
		this_a.row_id = refresh_queue.refresh_row_index;
		this_a.rank_id = refresh_queue.refresh_rank_index; // Does not matter
		refresh_queue.refresh_rank_index = (refresh_queue.refresh_rank_index
				+ 1) % config->rank_count;
		if (refresh_queue.refresh_rank_index == 0) { /* We rolled over so should increment row count */
			refresh_queue.refresh_row_index = (refresh_queue.refresh_row_index
					+ 1) % config->row_count;
		}
	}
	//	 fprintf(stdout," Get next refresh address chan %d rank %d row %d\n",this_a.chan_id,this_a.rank_id,this_a.row_id);
	return this_a;

}

bool Memory_Controller::is_refresh_pending() {

	Command * temp_c = refresh_queue.rq_head;
	refresh_queue.refresh_pending = FALSE;
	if (temp_c != NULL) {
		return true;
	}
	return false;
}

int Memory_Controller::check_cke_hi_pre(int rank_id) {
	//At this point, the rank  should be in precharge state!
	return rank[rank_id].cke_bit;
}

int Memory_Controller::check_cke_hi_act(int rank_id) {

	return rank[rank_id].cke_bit;

}

void Memory_Controller::update_cke_bit(tick_t now, int max_ranks) {
	int i;
	for (i = 0; i < max_ranks; i++) {
		if (rank[i].cke_done < now)
			rank[i].cke_bit = false;
	}
	return;
}

/**
 * Command sequences that are supported
 *
 * refresh
 * refresh_all
 * precharge->refresh
 * precharge_all->refresh_all
 *
 * TODO Make all commands special cases of the base command
 *
 * 1. REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK -> (Precharge_all) Refresh_all
 * 2. REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK -> Not known
 * 3. REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK -> (Precharge) Refresh
 *
 **/

Command* Memory_Controller::build_refresh_cmd_queue(tick_t now,
		DRAMaddress refresh_address, unsigned int refresh_id) {
	Command *command_queue = NULL;
	Command *temp_c;
	if (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK
			|| config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK
			|| config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
		if (config->row_buffer_management_policy != CLOSE_PAGE) {
			/* precharge all banks */
			command_queue = transaction_queue->add_command(now, command_queue,
					PRECHARGE_ALL, &refresh_address, refresh_id, 0, 0, 0);
		}
		/* activate this row on all banks */
		command_queue = transaction_queue->add_command(now, command_queue,
				REFRESH_ALL,&refresh_address, refresh_id, 0, 0, 0);
	} else if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK) {
		if (config->row_buffer_management_policy != CLOSE_PAGE) {
			/* precharge reqd bank only */
			command_queue = transaction_queue->add_command(now, command_queue,
					PRECHARGE,&refresh_address, refresh_id, 0, 0, 0);
		}
		command_queue = transaction_queue->add_command(now, command_queue,
				REFRESH,&refresh_address, refresh_id, 0, 0, 0);

	}
	temp_c = command_queue;
	while (temp_c != NULL) {
		temp_c->refresh = TRUE;
		temp_c = temp_c->next_c;
	}
	return command_queue;
}

void Memory_Controller::insert_refresh_transaction(int current_dram_time) {
	DRAMaddress refresh_address;
	Command * command_queue = NULL;
	Command * rq_temp_c = NULL;
	/** Check if the time for refresh has passed and t_rfc after it has gone by***/
	//fprintf(stdout, "Insert refresh: current_dram_time = %d, refresh_queue.size = %d\n", current_dram_time, refresh_queue.size);
	if (current_dram_time > (config->refresh_cycle_count
			+ refresh_queue.last_refresh_cycle) && (refresh_queue.size
			< MAX_REFRESH_QUEUE_DEPTH)) {
		//fprintf(stdout, "boom, inside\n");
		refresh_address = get_next_refresh_address();
		command_queue = build_refresh_cmd_queue(current_dram_time,
				refresh_address, refresh_queue.refresh_counter++);

		if (refresh_queue.rq_head == NULL) {
			refresh_queue.rq_head = command_queue;
		} else {
			rq_temp_c = refresh_queue.rq_head;
			while (rq_temp_c->rq_next_c != NULL) {
				rq_temp_c = rq_temp_c->rq_next_c;
			}
			rq_temp_c->rq_next_c = command_queue;
		}

		refresh_queue.refresh_pending = TRUE;
		refresh_queue.size++;
		refresh_queue.last_refresh_cycle = current_dram_time;
#ifdef DEBUG
		if (config->tq_info.get_transaction_debug()) {
			fprintf(
					stdout,"[%llu] Inserting Refresh Transaction %d Chan[%d] Rank [%d] Bank[%d] Row[%d]\n",
					current_dram_time, refresh_queue.refresh_counter - 1, id,
					refresh_address.rank_id, refresh_address.bank_id,
					refresh_address.row_id);
			Command* temp_c = command_queue;
			while (temp_c != NULL) {
				temp_c->print_command(current_dram_time);
				temp_c = temp_c->next_c;
			}
		}
#endif
	}

}

Command *Memory_Controller::next_cmd_in_transaction_queue(
		int transaction_selection_policy, int cmd_type, int chan_id,
		int rank_id, int bank_id, bool write_burst) {
	int i;
	int first_queued_command_found;
	Command *this_c;
	for (i = 0; i < transaction_queue->transaction_count; i++) {
		this_c = transaction_queue->entry[i].next_c;
		first_queued_command_found = FALSE;
		while ((this_c != NULL) && (first_queued_command_found == FALSE)) {
			if ((this_c->chan_id == chan_id) && (this_c->status == IN_QUEUE)) {
				first_queued_command_found = TRUE;
				if (((cmd_type == CAS && (this_c->command == CAS
						|| this_c->command == CAS_WRITE)) || (this_c->command
						== cmd_type))) { /* command not issued, see if we can issue it */
					if (cmd_type == PRECHARGE) {
						return this_c;
					} else {
						if (bank_id == -1) {
							if (this_c->rank_id == rank_id)
								return this_c;
						} else {
							if (this_c->rank_id == rank_id && this_c->bank_id
									== bank_id)
								return this_c;
						}
					}

				}
			}
			this_c = this_c->next_c;
		}
	}
	return NULL;
}

/* The update is done as follows
 * RAS ->
 *  switch current_rank to rank_id +1 mod num_ranks
 *  switch ranks bank to bank_id +1 mod num_banks
 * CAS ->
 *  switch ranks bank to bank_id +1 mod num_banks
 *  if bank rolled over switch current_rank to rank_id +1 mod num_ranks
 *  PRE
 *  nothing
 */
void Memory_Controller::update_rbrr_rank_bank_info(
		int transaction_selection_policy, int cmd, int rank_id, int bank_id,
		int cmd_index, int rank_count, int bank_count) {
	if (cmd == RAS) {
		sched_cmd_info.current_bank[cmd_index][rank_id] = (bank_id + 1)
				% bank_count;
		sched_cmd_info.current_rank[cmd_index] = (rank_id + 1) % rank_count;
	} else if (cmd == CAS) {
		sched_cmd_info.current_bank[cmd_index][rank_id] = (bank_id + 1)
				% bank_count;
		if (sched_cmd_info.current_bank[cmd_index][rank_id] == 0)
			sched_cmd_info.current_rank[cmd_index] = (rank_id + 1) % rank_count;
	}
}

Command *Memory_Controller::next_pre_in_transaction_queue(
		int transaction_selection_policy, int cmd_type, int chan_id,
		int rank_id, int bank_id, bool write_burst, int current_dram_time) {
	/* We scan till we get the first precharge that can be issued */
	int i;
	int first_queued_command_found;
	Command *this_c;
	for (i = 0; i < transaction_queue->transaction_count; i++) {
		this_c = transaction_queue->entry[i].next_c;
		first_queued_command_found = FALSE;
		while ((this_c != NULL) && (first_queued_command_found == FALSE)) {
			if ((this_c->chan_id == chan_id) && (this_c->status == IN_QUEUE)) {
				first_queued_command_found = TRUE;
				if (transaction_queue->entry[i].status == MEM_STATE_ISSUED) { /* command not issued, see if we can issue it */
					if (can_issue_command(i, this_c, current_dram_time)) {
						return this_c;
					}
				}
			}
			this_c = this_c->next_c;
		}
	}
	return NULL;
}

/*
 * This function returns the next refresh command that can be issued.
 * chan_id -> channel for which refresh command is being sought
 * transaction_selection_policy -> Transaction selection policy
 * RBRR stuff
 * command (RAS/PRE/CAS)
 * command_queue -> predetermined sequence
 */
Command * Memory_Controller::get_next_refresh_command_to_issue(tick_t now,
		int chan_id, int transaction_selection_policy, int command) {
	Command * rq_c, *this_c;
	int first_queued_command_found = FALSE;
	/* Traverse through the refresh queue and see if you can issue refresh
	 * commands in the same bundle **/
	if (row_command_bus_idle()) {
		//print_command(dram_system.current_dram_time,this_c);
		rq_c = refresh_queue.rq_head;
		while (rq_c != NULL) {
			this_c = rq_c;
			first_queued_command_found = FALSE;
			while (first_queued_command_found == FALSE && this_c != NULL) {
				if (this_c->status == IN_QUEUE) {
					first_queued_command_found = TRUE;

					if (can_issue_refresh_command(now, rq_c, this_c) == TRUE) {
						//			fprintf(stdout,"REF ");
						//			print_command(dram_system.current_dram_time,this_c);
						return this_c;
					}
				}
				this_c = this_c->next_c;
			}
			rq_c = rq_c->rq_next_c;
		}
	}
	return NULL;
}

int Memory_Controller::can_issue_refresh_command(tick_t now,
		Command *this_rq_c, Command *this_c) {

	switch (this_c->command) {

	case PRECHARGE:
		return can_issue_refresh_prec(now, this_rq_c, this_c);
	case PRECHARGE_ALL:
		return can_issue_refresh_prec_all(now, this_rq_c, this_c);
	case REFRESH_ALL:
		return can_issue_refresh_refresh_all(now, this_rq_c, this_c);
	case REFRESH:
		//  fprintf(stdout,">>>>> Can issue refresh command\n");
		//  print_command(dram_system.current_dram_time,this_c);
		return can_issue_refresh_refresh(now, this_rq_c, this_c);
	default:
		fprintf(stdout,"Not sure of command type, unable to issue");
		return FALSE;
	}
	return FALSE;

}

/**
 * 1. Close page -> Has to wait for the activate to complete
 *    Other -> Has to wait for activate to complete if it is the first command
 *    in the queue.
 * 2. Have to wait for previously issued transactions to complete.
 * 3. Has to wait for ras_done_time to lapse.
 */
int Memory_Controller::can_issue_refresh_prec(tick_t now, Command* this_rq_c,
		Command *this_c) {
	int j, pending_cas;
	Command *temp_c;

	tick_t cmd_bus_reqd_time = now
			+ (config->dram_type == FBD_DDR2 ? config->t_amb_up + config->t_bus
					* config->rank_count : 0);
	tick_t dram_reqd_time = cmd_bus_reqd_time + config->row_command_duration;

	int bank_id = this_c->bank_id;
	int rank_id = this_c->rank_id;
	pending_cas = FALSE;

	if (check_cmd_bus_required_time(this_c, cmd_bus_reqd_time) == FALSE)
		return FALSE;

	/** Takes care of RAS conflicts i.e. ensures tRAS is done **/
	if (rank[rank_id].bank[bank_id].ras_done_time > dram_reqd_time
			|| rank[rank_id].bank[bank_id].twr_done_time > dram_reqd_time) {
		return FALSE;
	}

	for (j = 0; j < transaction_queue->transaction_count; j++) { /* checked all preceeding transactions for not yet completed CAS command */
		temp_c = transaction_queue->entry[j].next_c;
		if ((temp_c->rank_id == rank_id) && (temp_c->bank_id == bank_id)
				&& transaction_queue->entry[j].status == MEM_STATE_ISSUED) {
			while (temp_c != NULL) {
				if (temp_c->status == IN_QUEUE)
					return FALSE;
				if (temp_c->rank_done_time > dram_reqd_time)
					return FALSE;
				temp_c = temp_c->next_c;
			}
		}
	}

	return TRUE;
}

/*** For the DATA CAS_WRITE transistions are
 *
 * LINK_COMMAND_TRANSMISSION ---------------> AMB_PROCESSING -------------------> MEM_STATE_COMPLETED
 * t(bundle) + t(bus)					   t(ambup)
 * Note that the actual command is issued when the write data is received and
 * this is just a place holder for the command.
 * ***/

void Memory_Controller::data_transition(Command *this_c, int tran_index,
		int chan_id, int rank_id, int bank_id, int row_id, char *debug_string,
		int current_dram_time) {
	Transaction *this_t;
	this_t = &(transaction_queue->entry[tran_index]);
	if ((this_c->status == LINK_COMMAND_TRANSMISSION)
			&& (this_c->link_comm_tran_comp_time <= current_dram_time)) {
		this_c->status = AMB_PROCESSING;
#ifdef DEBUG
		if (config->cas_wave_debug() || config->wave_debug())
			build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif

	} else if ((this_c->status == AMB_PROCESSING)
			&& (this_c->amb_proc_comp_time <= current_dram_time)) {
		this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
		if (config->cas_wave_debug() || config->wave_debug())
			build_wave(&debug_string[0], tran_index, this_c,
					MEM_STATE_COMPLETED);
#endif
	}

}

/***
 * 1. Ras done time for all banks are honored.
 * 2. Check if there are previous commands in flight that have to complete.
 * Need to add more sophistication into the CAS checks.
 */

int Memory_Controller::can_issue_refresh_prec_all(tick_t now,
		Command* this_rq_c, Command *this_c) {
	int i, j, pending_cas;
	Command *temp_c;

	tick_t dram_cmd_bus_reqd_time = now
			+ (config->dram_type == FBD_DDR2 ? config->t_bus
					* config->rank_count + config->t_amb_up : 0);
	tick_t dram_reqd_time = dram_cmd_bus_reqd_time
			+ config->row_command_duration;

	pending_cas = FALSE;

	if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
		if (config->dram_type == FBD_DDR2 && dram_cmd_bus_reqd_time
				< rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time) {
			return FALSE;
		}
		for (j = 0; j < config->bank_count; j++) {
			if (dram_reqd_time < rank[this_c->rank_id].bank[j].ras_done_time
					|| dram_reqd_time
							< rank[this_c->rank_id].bank[j].twr_done_time) {
#if DEBUG
				if (config->get_ref_tran_watch(this_c->tran_id)) {
					fprintf(
							stdout,"[%llu] PREC_ALL ras_done_time(%llu) > dram_reqd_time(%llu) rank(%d) bank(%d)",
							now, rank[this_c->rank_id].bank[j].ras_done_time,
							dram_reqd_time, this_c->rank_id, j);
				}
#endif
				return FALSE; /* t_ras not yet satisfied for this bank */
			}
		}
	} else {
		for (i = 0; i < config->rank_count; i++) {
			if (config->dram_type == FBD_DDR2 && dram_cmd_bus_reqd_time
					< rank[i].my_dimm->fbdimm_cmd_bus_completion_time) {
#if DEBUG
				if (config->get_ref_tran_watch(this_c->tran_id)) {
					fprintf(stdout,"[%llu] PREC_ALL Command bus not available",
							now);
				}
#endif
				return FALSE;
			}
			if (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
				for (j = 0; j < config->bank_count; j++) {
					if (dram_reqd_time < rank[i].bank[j].ras_done_time
							|| dram_reqd_time < rank[i].bank[j].twr_done_time) {
#if DEBUG
						if (config->get_ref_tran_watch(this_c->tran_id)) {
							fprintf(
									stdout,"[%llu] PREC_ALL Bank not ready in time ",
									now);
						}
#endif
						return FALSE; /* t_ras not yet satisfied for this bank */
					}

				}
			} else {
				if (rank[i].bank[this_c->bank_id].ras_done_time
						> dram_reqd_time
						|| rank[i].bank[this_c->bank_id].twr_done_time
								> dram_reqd_time) {
#if DEBUG
					if (config->get_ref_tran_watch(this_c->tran_id)) {
						fprintf(
								stdout,"[%llu] PREC_ALL Bank not ready in time ",
								now);
					}
#endif
					return FALSE; /* t_ras not yet satisfied for this bank */
				}
			}
		}
	}
	for (j = 0; j < transaction_queue->transaction_count; j++) { /* checked all preceeding transactions for not yet completed CAS command */
		temp_c = transaction_queue->entry[j].next_c;
		if ((transaction_queue->entry[j].status == MEM_STATE_ISSUED
		// || transaction_queue->entry[j].status == MEM_STATE_DATA_ISSUED
				|| transaction_queue->entry[j].status
						== MEM_STATE_CRITICAL_WORD_RECEIVED)
				&& (config->refresh_policy
						== REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK
						|| (config->refresh_policy
								== REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK
								&& temp_c->bank_id == this_c->bank_id)
						|| (config->refresh_policy
								== REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK
								&& temp_c->rank_id == this_c->rank_id))) {
			while (temp_c != NULL) {
				if (temp_c->status == IN_QUEUE || (temp_c->rank_done_time
						> dram_reqd_time)) {
#ifdef DEBUG
					if (config->get_ref_tran_watch(this_c->tran_id)) {
						fprintf(
								stdout,"[%llu] PREC_ALL CAS not complete for issued tran (%d) ",
								now, transaction_queue->entry[j].status);
						temp_c->print_command(now);
					}
#endif
					return FALSE; /* PRECHARGE has to be suppressed until all previous CAS complete */
				}
				temp_c = temp_c->next_c;
			}
		}
	}
	return TRUE;
}

/***
 * To check :
 *  Cmd Bus is available
 *  Banks are precharged.
 *  t_rfc is satisfied.
 */
int Memory_Controller::can_issue_refresh_refresh(tick_t now,
		Command *this_rq_c, Command *this_c) {

	int rank_id, bank_id;
	tick_t dram_reqd_time;
	tick_t cmd_bus_reqd_time;

	int i;
	Command *temp_c;

	if (config->dram_type == FBD_DDR2) {
		cmd_bus_reqd_time = now + config->t_bus * config->rank_count
				+ config->t_bundle + config->t_amb_up;
	} else {
		cmd_bus_reqd_time = now;
	}
	dram_reqd_time = cmd_bus_reqd_time + config->row_command_duration;
	rank_id = this_c->rank_id;
	bank_id = this_c->bank_id;

	if ((rank[rank_id].bank[bank_id].rp_done_time
			>= rank[rank_id].bank[bank_id].ras_done_time)
			&& (rank[rank_id].bank[bank_id].rp_done_time <= dram_reqd_time)) {
	} else {
		if (config->get_ref_tran_watch(this_c->tran_id)) {
			fprintf(
					stdout,"%llu  Failed to issue refresh due to bank status %d rp_done_time %llu \n",
					now, rank[rank_id].bank[bank_id].status,
					rank[rank_id].bank[bank_id].rp_done_time);
		}
		return FALSE;
	}
	if (config->dram_type == FBD_DDR2 && check_cmd_bus_required_time(this_c,
			cmd_bus_reqd_time) == FALSE) {
		if (config->get_ref_tran_watch(this_c->tran_id)) {
			fprintf(
					stdout,"%llu  Failed to issue refresh due to conflict with cmd bus %llu \n",
					now, cmd_bus_reqd_time);
		}
		return FALSE;
	}

	for (i = 0; i < transaction_queue->transaction_count; i++) {
		temp_c = transaction_queue->entry[i].next_c;
		if (temp_c->rank_id == rank_id && temp_c->bank_id == bank_id
				&& transaction_queue->entry[i].status == MEM_STATE_ISSUED) {
			while (temp_c != NULL) {
				if (temp_c->status == IN_QUEUE) {
					if (config->get_ref_tran_watch(this_c->tran_id)) {
						fprintf(
								stdout,"%llu  Failed to issue refresh due to conflict with issued transaction %llu \n",
								now, temp_c->tran_id);
					}
					return FALSE;
				}
				temp_c = temp_c->next_c;
			}
		}
	}
	/** Check if your bank is precharged - open page policy **/
	if (config->row_buffer_management_policy != CLOSE_PAGE) {
		temp_c = this_rq_c;
		if (temp_c->status == IN_QUEUE && temp_c->command == PRECHARGE) { /** Has to be the case **/
			if (config->get_ref_tran_watch(this_c->tran_id)) {
				fprintf(
						stdout,"%llu  Failed to issue refresh due to conflict with precharge \n",
						now);
			}
			return FALSE;
		}
	}
	return TRUE;
}

/***
 * To check :
 *  Cmd Bus is available
 *  Banks are precharged.
 *  t_rfc is satisfied.
 */
int Memory_Controller::can_issue_refresh_refresh_all(tick_t now,
		Command *this_rq_c, Command *this_c) {

	tick_t dram_reqd_time;
	tick_t cmd_bus_reqd_time;
	int i, j;
	Command *temp_c;

	if (config->dram_type == FBD_DDR2) {
		cmd_bus_reqd_time = now + config->t_bus * config->rank_count
				+ config->t_bundle + config->t_amb_up;
	} else {
		cmd_bus_reqd_time = now;
	}
	dram_reqd_time = cmd_bus_reqd_time + config->row_command_duration;

	/** Check if your bank is precharged - open page policy **/
	if (config->row_buffer_management_policy != CLOSE_PAGE) {
		temp_c = this_rq_c;
		if ((temp_c->command == PRECHARGE || temp_c->command == PRECHARGE_ALL)
				&& temp_c->dram_proc_comp_time > dram_reqd_time) { /** Has to be the case **/
#ifdef DEBUG
			if (config->get_ref_tran_watch(this_c->tran_id)) {
				fprintf(
						stdout,"%llu  Failed to issue refresh since PRECHARGE is not done \n",
						now);
			}
#endif
			return FALSE;
		}
	}

	if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
		if (config->dram_type == FBD_DDR2
				&& rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time
						> cmd_bus_reqd_time) {
#ifdef DEBUG
			if (config->get_ref_tran_watch(this_c->tran_id)) {
				fprintf(
						stdout,"%llu  Failed to issue refresh due to bus done time %llu > bus reqd time %llu \n",
						now,
						rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time,
						cmd_bus_reqd_time);
			}
#endif
			return FALSE; /* t_ras not yet satisfied for this bank */
		}
		for (j = 0; j < config->bank_count; j++) {
			if (rank[this_c->rank_id].bank[j].rp_done_time
					< rank[this_c->rank_id].bank[j].ras_done_time
					|| dram_reqd_time
							< rank[this_c->rank_id].bank[j].rp_done_time) {
#ifdef DEBUG
				if (config->get_ref_tran_watch(this_c->tran_id)) {
					fprintf(
							stdout,"%llu  aFailed to issue refresh due to bank(r%db%d) ras_done_time %llu rp_done_time %llu dram_reqd_time %llu %d %d\n",
							now,
							this_c->rank_id,
							j,
							rank[this_c->rank_id].bank[j].ras_done_time,
							rank[this_c->rank_id].bank[j].rp_done_time,
							dram_reqd_time,
							rank[this_c->rank_id].bank[j].rp_done_time
									< rank[this_c->rank_id].bank[j].ras_done_time,
							dram_reqd_time
									< rank[this_c->rank_id].bank[j].rp_done_time);
				}
#endif
				return FALSE; /* t_ras not yet satisfied for this bank */
			}

		}
	} else {
		for (i = 0; i < config->rank_count; i++) {
			if (config->dram_type == FBD_DDR2
					&& rank[i].my_dimm->fbdimm_cmd_bus_completion_time
							> cmd_bus_reqd_time) {
#ifdef DEBUG
				if (config->get_ref_tran_watch(this_c->tran_id)) {
					fprintf(
							stdout,"%llu  Failed to issue refresh due to bus reqd time %llu < bus available time %llu \n",
							now,
							cmd_bus_reqd_time,
							rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time);
				}
#endif
				return FALSE;
			}
			if (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
				for (j = 0; j < config->bank_count; j++) {
					if (rank[i].bank[j].rp_done_time
							>= rank[i].bank[j].ras_done_time && (dram_reqd_time
							> rank[i].bank[j].rp_done_time)) {
					} else {
						if (config->get_ref_tran_watch(this_c->tran_id)) {
							fprintf(
									stdout,"%llu  bFailed to issue refresh due to bank(r%db%d) status %d rp_done_time %llu \n",
									now, i, j, rank[i].bank[j].status,
									rank[i].bank[j].rp_done_time);
						}
						return FALSE; /* t_ras not yet satisfied for this bank */
					}
				}
			} else {
				if (rank[i].bank[this_c->bank_id].rp_done_time
						>= rank[i].bank[this_c->bank_id].ras_done_time
						&& (dram_reqd_time
								> rank[i].bank[this_c->bank_id].rp_done_time)) {
				} else {
					if (config->get_ref_tran_watch(this_c->tran_id)) {
						fprintf(
								stdout,"%llu  cFailed to issue refresh due to bank(r%db%d) status %d rp_done_time %llu \n",
								now,
								this_c->rank_id,
								this_c->bank_id,
								rank[this_c->rank_id].bank[this_c->bank_id].status,
								rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time);
					}
					return FALSE; /* t_ras not yet satisfied for this bank */
				}
			}

		}
	}
	for (i = 0; i < transaction_queue->transaction_count; i++) {
		temp_c = transaction_queue->entry[i].next_c;
		if (transaction_queue->entry[i].status == MEM_STATE_ISSUED) {
			while (temp_c != NULL) {
				if (temp_c->status == IN_QUEUE) {
					if (((config->refresh_policy
							== REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK
							|| (config->refresh_policy
									== REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK
									&& temp_c->bank_id == this_c->bank_id)
							|| (config->refresh_policy
									== REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK
									&& temp_c->rank_id == this_c->rank_id))))
						return FALSE;
				}
				temp_c = temp_c->next_c;
			}
		}
	}
	/** Check if your precharge is issued - open page policy **/
	if (config->row_buffer_management_policy != CLOSE_PAGE) {
		temp_c = this_rq_c;
		if ((temp_c->command == PRECHARGE || temp_c->command == PRECHARGE_ALL)
				&& temp_c->status == IN_QUEUE) { /** Has to be the case **/
#ifdef DEBUG
			if (config->get_ref_tran_watch(this_c->tran_id)) {
				fprintf(
						stdout,"%llu  Failed to issue refresh since PRECHARGE is not done \n",
						now);
			}
#endif
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * Check for refresh queue status first.
 * - Refresh policies possible -> refresh top priority/ refresh oppourtunistic
 *   - returns is_refresh_queue_full
 *   - Get refresh_command that can go
 *   	- Any rank/bank/cmd constraints need to be satisfied
 *
 *   - Find first command that can go
 *   	- satisfying possible constraints
 *   		- cmd type
 *   		- rank/bank
 *   		- arrival order
 */

void Memory_Controller::issue_new_commands(tick_t now, char *debug_string) {
	int i = 0;
	Command *this_c = NULL;

	int transaction_selection_policy;
	int nextRBRR_rank, nextRBRR_bank, nextRBRR_cmd;

	int command_issued = FALSE;
	Command *refresh_command_to_issue = NULL;
	int refresh_queue_full = FALSE;

	transaction_selection_policy = config->get_transaction_selection_policy();
	Command *cmd_issued = NULL;
	if (config->auto_refresh_enabled == TRUE) {
		refresh_queue_full = refresh_queue.is_refresh_queue_half_full();
		refresh_command_to_issue = get_next_refresh_command_to_issue(now, id,
				transaction_selection_policy, next_command);
		if (refresh_command_to_issue != NULL && ((config->refresh_issue_policy
				== REFRESH_OPPORTUNISTIC && refresh_queue_full)
				|| config->refresh_issue_policy == REFRESH_HIGHEST_PRIORITY)) {
			/* Is refresh queue full ? -> then issue
			 * Find the first refresh command that you can issue -> only if no
			 * read/write command exists do you issue */
			issue_cmd(now, refresh_command_to_issue, NULL);
			cmd_issued = refresh_command_to_issue;
			command_issued = TRUE;
		}
	}
	// Do something for aging transactions
	// Note that there can be transactions for mp/lp which do not get done
	// despite being older ... also check for issued stuff which needs to get
	// done
	if (transaction_queue->transaction_count > 0 && (now
			- transaction_queue->entry[0].arrival_time
			> config->arrival_threshold)) {
		// Check if that
		if (col_command_bus_idle() || row_command_bus_idle()) {
			Command *this_c = transaction_queue->entry[0].next_c;
			while (this_c != NULL && cmd_issued == NULL) {
				if (this_c->status == IN_QUEUE && can_issue_command(0, this_c, now)) {
					cmd_issued = this_c;
					// We need to clean up the command queue in case we are
					// using a CAS but
					//         // there is a PRE/RAS earlier to it.

					if (config->row_buffer_management_policy != CLOSE_PAGE
							&& cmd_issued->cmd_id != 0
							&& transaction_queue->entry[0].status
									!= MEM_STATE_ISSUED && (cmd_issued->command
							!= RAS || cmd_issued->command != PRECHARGE)) {
						transaction_queue->collapse_cmd_queue(
								&transaction_queue->entry[0], cmd_issued);
					}
					issue_cmd(now, cmd_issued, &transaction_queue->entry[0]);
					command_issued = TRUE;

				}
				this_c = this_c->next_c;
			}

		}
	}

	if (transaction_selection_policy == WANG) {
		/* The process of finding the next WANG command is as follows
		 * 1. Find the next valid command.
		 * 2. Find the valid rank / bank combo
		 * 3. Check for a valid combination.
		 * 4. If valid or invalid -> update the next valid command/ rank / bank
		 * numbers to maintain the sequence.
		 * 5. How do we put in the DQS handling capability?
		 * 6. How about refresh?
		 */
		this_c = NULL;
		int num_scans = 0; /* How do we fix this? */
		int row_bus_free = row_command_bus_idle();
		int col_bus_free = col_command_bus_idle();
		bool pre_chked = false;
		if (row_bus_free || col_bus_free) {
			while (cmd_issued == NULL && num_scans < config->rank_count
					* config->bank_count * 3) {
				this_c = NULL;
				nextRBRR_cmd = sched_cmd_info.current_cmd;
				int cmd_index = sched_cmd_info.get_rbrr_cmd_index();
				nextRBRR_rank = sched_cmd_info.current_rank[cmd_index];
				nextRBRR_bank
						= sched_cmd_info.current_bank[cmd_index][nextRBRR_rank];
				if ((nextRBRR_cmd == CAS && col_bus_free) || ((nextRBRR_cmd
						== PRECHARGE || nextRBRR_cmd == RAS) && row_bus_free)) { /* No need to do any of this */
					if (nextRBRR_cmd == PRECHARGE) {
						if (!pre_chked)
							this_c = next_pre_in_transaction_queue(
									transaction_selection_policy, nextRBRR_cmd,
									id, nextRBRR_rank, nextRBRR_bank, false,
									now);
						else
							this_c = NULL;
						pre_chked = true;
					} else {
						this_c = next_cmd_in_transaction_queue(
								transaction_selection_policy, nextRBRR_cmd, id,
								nextRBRR_rank, nextRBRR_bank, false);
					}
					int tran_index = 0;
					if (this_c != NULL) {
						transaction_queue->get_transaction_index(
								this_c->tran_id);
					}
					if (this_c != NULL && (nextRBRR_cmd == PRECHARGE
							|| (nextRBRR_cmd != PRECHARGE))) {
						if (can_issue_command(tran_index, this_c, now)) {
							issue_cmd(now, this_c,
									&transaction_queue->entry[tran_index]);
							command_issued = TRUE;
							cmd_issued = this_c;
						}
					}

					/* Update the rank and bank stuff */
				}
				update_rbrr_rank_bank_info(transaction_selection_policy,
						nextRBRR_cmd, nextRBRR_rank, nextRBRR_bank, cmd_index,
						config->rank_count, config->bank_count);
				sched_cmd_info.set_rbrr_next_command(nextRBRR_cmd);
				num_scans++;
			}
		}
	} else if (transaction_selection_policy == FCFS
			|| transaction_selection_policy == GREEDY
			|| transaction_selection_policy == OBF
			|| transaction_selection_policy == RIFF) {
		/* In-order
		 * Check if a transaction can be scheduled only if all transactions before
		 * it have been scheduled.
		 * */
		/* RIFF In this policy -> you try to schedule the reads first - then you try to
		 * schedule a write
		 * Refinement would be to do write-bursting; pend all writes and then do
		 * them back-to-back*/
		if (col_command_bus_idle() || row_command_bus_idle()) {
			cmd_issued = get_next_cmd_to_issue(now,
					transaction_selection_policy);
			if (cmd_issued != NULL) {
				int tran_index = transaction_queue->get_transaction_index(
						cmd_issued->tran_id);
				Transaction * this_t = &transaction_queue->entry[tran_index];

				if (config->row_buffer_management_policy != CLOSE_PAGE
						&& cmd_issued->cmd_id != 0 && this_t->status
						!= MEM_STATE_ISSUED && (cmd_issued->command
						!= PRECHARGE)) {
					transaction_queue->collapse_cmd_queue(this_t, cmd_issued);
				}
				issue_cmd(now, cmd_issued,
						&transaction_queue->entry[tran_index]);
				command_issued = TRUE;
				// Note that if posted cas works then you actually issue the CAS as
				// well
				if (config->posted_cas_flag && cmd_issued->command == RAS) {
					cmd_issued->next_c->posted_cas = true;
					issue_cmd(now, cmd_issued->next_c, this_t);
				}
			} else if (transaction_selection_policy == OBF
					|| transaction_selection_policy == RIFF) {
				cmd_issued = get_next_cmd_to_issue(now, FCFS);
				if (cmd_issued != NULL) {
					int tran_index = transaction_queue->get_transaction_index(
							cmd_issued->tran_id);
					Transaction * this_t =
							&transaction_queue->entry[tran_index];
					if (config->row_buffer_management_policy != CLOSE_PAGE
							&& cmd_issued->cmd_id != 0 && this_t->status
							!= MEM_STATE_ISSUED && (cmd_issued->command
							!= PRECHARGE)) {
						transaction_queue->collapse_cmd_queue(this_t,
								cmd_issued);
					}
					issue_cmd(now, cmd_issued,
							&transaction_queue->entry[tran_index]);
					command_issued = TRUE;
					// Note that if posted cas works then you actually issue the CAS as
					// well
					if (config->posted_cas_flag && cmd_issued->command == RAS) {
						cmd_issued->next_c->posted_cas = true;
						issue_cmd(now, cmd_issued->next_c, this_t);
					}

				}
			}
		}
	} else if (transaction_selection_policy == MOST_PENDING
			|| transaction_selection_policy == LEAST_PENDING) {
		if (col_command_bus_idle() || row_command_bus_idle()) {
			cmd_issued = get_next_cmd_to_issue(now,
					transaction_selection_policy);
			if (cmd_issued != NULL) {
				// We need to clean up the command queue in case we are using a CAS but
				// there is a PRE/RAS earlier to it.
				int tran_index = transaction_queue->get_transaction_index(
						cmd_issued->tran_id);
				Transaction * this_t = &transaction_queue->entry[tran_index];

				if (config->row_buffer_management_policy != CLOSE_PAGE
						&& cmd_issued->cmd_id != 0 && this_t->status
						!= MEM_STATE_ISSUED && (cmd_issued->command
						!= PRECHARGE)) {
					transaction_queue->collapse_cmd_queue(this_t, cmd_issued);
				}
				issue_cmd(now, cmd_issued, this_t);
				command_issued = TRUE;
				// Note that if posted cas works then you actually issue the CAS as
				// well
				if (config->posted_cas_flag && cmd_issued->command == RAS) {
					cmd_issued->next_c->posted_cas = true;
					issue_cmd(now, cmd_issued->next_c, this_t);
				}
			}
		}
	}
	if (command_issued == FALSE) {
		if (refresh_command_to_issue != NULL) {
			issue_cmd(now, refresh_command_to_issue, NULL);
			command_issued = TRUE;
			cmd_issued = refresh_command_to_issue;
		} else {
			if (refresh_queue.is_refresh_queue_empty()) {
				// We issue a refresh command based on which rank is not busy?
			}
		}
	}
	if (cmd_issued != NULL) {
		if (config->wave_debug()) {
			build_wave(debug_string, i, cmd_issued, MEM_STATE_SCHEDULED);
			if (cmd_issued->command == RAS && !cmd_issued->refresh
					&& config->posted_cas_flag)
				build_wave(debug_string, i, cmd_issued->next_c,
						MEM_STATE_SCHEDULED);
		}
	}
}

/*  This function will set up bundles and the timing
 *  related to the overall bundle.
 */

bool Memory_Controller::is_bank_open(int rank_id, int bank_id, int row_id) {
	if (rank[rank_id].bank[bank_id].row_id == row_id
			&& rank[rank_id].bank[bank_id].ras_done_time
					> rank[rank_id].bank[bank_id].rp_done_time)
		return true;
	else
		return false;

}

Command * Memory_Controller::get_next_cmd_to_issue(tick_t now,
		int trans_sel_policy) {

	/* In-order
	 * Check if a transaction can be scheduled only if all transactions before
	 * it have been scheduled.
	 * */
	int i;
	Command *this_c = NULL;
	Command *cmd_issued = NULL;
	bool first_queued_command_found = FALSE; /* make sure command sequences execute in strict order within a transaction */
	if (trans_sel_policy == FCFS || trans_sel_policy == GREEDY
			|| trans_sel_policy == OBF) {
		bool first_queued_transaction = FALSE;

		for (i = 0; (i < transaction_queue->transaction_count) && (cmd_issued
				== NULL && (first_queued_transaction == FALSE)); i++) {
			bool attempt_schedule = false; // Stuff added for the update latency info
			bool issue_tran = false;
			this_c = transaction_queue->entry[i].next_c;
			first_queued_command_found = FALSE; /* make sure command sequences execute in strict order within a transaction */
			if (transaction_queue->entry[i].status == MEM_STATE_SCHEDULED
					&& trans_sel_policy == FCFS)
				first_queued_transaction = TRUE;
			while ((this_c != NULL) && (first_queued_command_found == FALSE)
					&& ((trans_sel_policy != OBF) || (trans_sel_policy == OBF
							&& is_bank_open(this_c->rank_id, this_c->bank_id,
									this_c->row_id)))) {
				if ((this_c->chan_id == id) && (this_c->status == IN_QUEUE)) { /* command has not yet issued, lets see if we can issue it */
					if (config->row_buffer_management_policy == CLOSE_PAGE
							|| (this_c->command == CAS || this_c->command
									== CAS_WRITE || this_c->command
									== CAS_WRITE_AND_PRECHARGE
									|| this_c->command == CAS_AND_PRECHARGE)) {
						first_queued_command_found = TRUE;
					}
					attempt_schedule = true;
					if (can_issue_command(i, this_c, now) == TRUE
							&& is_cmd_bus_idle(this_c)) {
						cmd_issued = this_c;
						issue_tran = true;
						//if (wave_debug()) build_wave(&debug_string[0], i, this_c, MEM_STATE_SCHEDULED);
					}
				}
				this_c = this_c->next_c;
			}
		}
	} else if (trans_sel_policy == RIFF) {
		for (i = 0; (i < transaction_queue->transaction_count) && (cmd_issued
				== NULL); i++) {
			bool attempt_schedule = false; // Stuff added for the update latency info
			bool issue_tran = false;
			this_c = transaction_queue->entry[i].next_c;
			first_queued_command_found = FALSE; /* make sure command sequences execute in strict order within a transaction */
			if (transaction_queue->entry[i].transaction_type
					== MEMORY_READ_COMMAND
					|| transaction_queue->entry[i].transaction_type
							== MEMORY_IFETCH_COMMAND) {
				while ((this_c != NULL)
						&& (first_queued_command_found == FALSE)) {
					if ((this_c->chan_id == id) && (this_c->status == IN_QUEUE)) { /* command has not yet issued, lets see if we can issue it */
						/* Check if this is a CAS -> yes : try to issue;
						 * 							no : skip till you get CAS */
						attempt_schedule = true;
						if (config->row_buffer_management_policy == CLOSE_PAGE
								|| (this_c->command == CAS || this_c->command
										== CAS_WRITE || this_c->command
										== CAS_WRITE_AND_PRECHARGE
										|| this_c->command == CAS_AND_PRECHARGE)) {
							first_queued_command_found = TRUE;
						}
						if (can_issue_command(i, this_c, now) == TRUE
								&& is_cmd_bus_idle(this_c)) {
							cmd_issued = this_c;
							issue_tran = true;
						}
					}

					this_c = this_c->next_c;
				}
			}
		}
	} else if (trans_sel_policy == MOST_PENDING || trans_sel_policy
			== LEAST_PENDING) {
		// Do the check for Issued but not completed transactions
		for (i = 0; (i < transaction_queue->transaction_count) && (cmd_issued
				== NULL); i++) {
			bool attempt_schedule = false; // Stuff added for the update latency info
			bool issue_tran = false;
			this_c = transaction_queue->entry[i].next_c;
			first_queued_command_found = FALSE; /* make sure command sequences execute in strict order within a transaction */
			if (transaction_queue->entry[i].status == MEM_STATE_ISSUED) {
				while ((this_c != NULL)
						&& (first_queued_command_found == FALSE)) {
					if ((this_c->chan_id == id) && (this_c->status == IN_QUEUE)) { /* command has not yet issued, lets see if we can issue it */
						/* Check if this is a CAS -> yes : try to issue;
						 * 							no : skip till you get CAS */
						attempt_schedule = true;
						if (config->row_buffer_management_policy == CLOSE_PAGE
								|| (this_c->command == CAS || this_c->command
										== CAS_WRITE || this_c->command
										== CAS_WRITE_AND_PRECHARGE
										|| this_c->command == CAS_AND_PRECHARGE)) {
							first_queued_command_found = TRUE;
						}
						if (can_issue_command(i, this_c, now) == TRUE
								&& is_cmd_bus_idle(this_c)) {
							cmd_issued = this_c;
							issue_tran = true;
						}
					}

					this_c = this_c->next_c;
				}
			}
		}

		if (trans_sel_policy == MOST_PENDING) {
			/* Need to issue commands with most CAS accesses */
			Transaction *this_t;
			int j;
			for (i = 0; i < pending_queue.queue_size && cmd_issued == NULL; i++) {
				for (j = 0; j < pending_queue.pq_entry[i].transaction_count
						&& cmd_issued == NULL; j++) {
					int tindex = transaction_queue->get_transaction_index(
							pending_queue.pq_entry[i].transaction_list[j]);
					this_t = &transaction_queue->entry[tindex];
					this_c = this_t->next_c;
					first_queued_command_found = FALSE;
					bool attempt_schedule = false; // Stuff added for the update latency info
					bool issue_tran = false;
					while ((this_c != NULL) && (first_queued_command_found
							== FALSE)) {
						if ((this_c->status == IN_QUEUE)) { /* command has not yet issued, lets see if we can issue it */
							/* Check if this is a CAS -> yes : try to issue;
							 * 							no : skip till you get CAS */
							attempt_schedule = true;
							if (config->row_buffer_management_policy
									== CLOSE_PAGE || (this_c->command == CAS
									|| this_c->command == CAS_WRITE
									|| this_c->command
											== CAS_WRITE_AND_PRECHARGE
									|| this_c->command == CAS_AND_PRECHARGE)) {
								first_queued_command_found = TRUE;
							}
							if (can_issue_command(tindex, this_c, now) == TRUE
									&& is_cmd_bus_idle(this_c)) {
								cmd_issued = this_c;
								issue_tran = true;
							}
						}

						this_c = this_c->next_c;
					}

				}

			}
		} else if (trans_sel_policy == LEAST_PENDING) {
			/* Need to issue commands with least CAS accesses */
			Transaction *this_t;
			int j;
			for (i = pending_queue.queue_size - 1; cmd_issued == NULL && i >= 0; i--) {
				for (j = 0; j < pending_queue.pq_entry[i].transaction_count
						&& cmd_issued == NULL; j++) {
					int tindex = transaction_queue->get_transaction_index(
							pending_queue.pq_entry[i].transaction_list[j]);
					this_t = &transaction_queue->entry[tindex];
					this_c = this_t->next_c;
					first_queued_command_found = FALSE;
					bool attempt_schedule = false; // Stuff added for the update latency info
					bool issue_tran = false;
					while ((this_c != NULL) && (first_queued_command_found
							== FALSE)) {

						if ((this_c->status == IN_QUEUE)) { /* command has not yet issued, lets see if we can issue it */
							/* Check if this is a CAS -> yes : try to issue;
							 * 							no : skip till you get CAS */

							if (config->row_buffer_management_policy
									== CLOSE_PAGE || (this_c->command == CAS
									|| this_c->command == CAS_WRITE
									|| this_c->command
											== CAS_WRITE_AND_PRECHARGE
									|| this_c->command == CAS_AND_PRECHARGE)) {
								first_queued_command_found = TRUE;
							}
							attempt_schedule = true;
							if (can_issue_command(tindex, this_c, now) == TRUE
									&& is_cmd_bus_idle(this_c)) {
								cmd_issued = this_c;
								issue_tran = true;
							}
						}
						this_c = this_c->next_c;
					}
				}
			}
		}
	}
	return cmd_issued;
}

void Memory_Controller::issue_cas(tick_t now, Command *this_c) {
	this_c->status = LINK_COMMAND_TRANSMISSION;
	if (config->dram_type == FBD_DDR2) {
		/**lock_buffer(this_c,chan_id);
		 this_c->link_comm_tran_comp_time = now + config->t_bundle +
		 (config->var_latency_flag ?config->t_bus*(this_c->rank_id): config->t_bus * (config->rank_count - 1));
		 this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time +
		 config->t_amb_up +
		 (this_c->posted_cas ? config->row_command_duration: 0);
		 this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time +
		 config->col_command_duration;
		 this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time +
		 config->t_cac +
		 (config->posted_cas_flag? config->t_al :0);
		 this_c->dimm_data_tran_comp_time = this_c->dram_proc_comp_time +
		 config->t_burst;
		 this_c->amb_down_proc_comp_time = 0;
		 this_c->link_data_tran_comp_time = 0;
		 this_c->rank_done_time = this_c->dimm_data_tran_comp_time;
		 this_c->completion_time = this_c->dimm_data_tran_comp_time;
		 rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;
		 rank[this_c->rank_id].my_dimm->data_bus_completion_time = this_c->dimm_data_tran_comp_time;
		 if (amb_buffer_debug())
		 print_buffer_contents(chan_id, this_c->rank_id);**/

	} else {
		int tran_index;
		tran_index = transaction_queue->get_transaction_index(this_c->tran_id);
		transaction_queue->entry[tran_index].status = MEM_STATE_ISSUED;
		transaction_queue->entry[tran_index].issued_cas = true;
		set_col_command_bus(this_c->command, now);
		this_c->link_comm_tran_comp_time = now
				+ (this_c->posted_cas ? config->row_command_duration : 0)
				+ config->col_command_duration;
		this_c->amb_proc_comp_time = 0;
		this_c->dimm_comm_tran_comp_time = 0;
		this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time
				+ config->t_cac + (config->posted_cas_flag ? config->t_al : 0);
		this_c->dimm_data_tran_comp_time = 0;
		this_c->amb_down_proc_comp_time = 0;
		this_c->link_data_tran_comp_time = this_c->dram_proc_comp_time
				+ config->t_burst;
		// Set up Data bus time
		data_bus.completion_time = this_c->link_data_tran_comp_time;
		data_bus.current_rank_id = this_c->rank_id;
		//now + config->t_cas +
		//(config->posted_cas_flag? (config->t_al + config->row_command_duration):0) + config->t_burst;

		this_c->rank_done_time = this_c->link_data_tran_comp_time;
		if (this_c->command == CAS_AND_PRECHARGE) {
			// Check for t_ras time now

			rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time = MAX(rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time,
					this_c->link_data_tran_comp_time + config->t_rtp)
					+ config->t_rp;
		} else {
		}
		this_c->completion_time = this_c->rank_done_time;
	}
	this_c->status = LINK_COMMAND_TRANSMISSION;
}

void Memory_Controller::issue_cas_write(tick_t now, Command *this_c) {
	this_c->status = LINK_COMMAND_TRANSMISSION;
	if (config->dram_type == FBD_DDR2) {
		/**this_c->link_comm_tran_comp_time = now +
		 config->t_bundle +
		 (config->var_latency_flag ?config->t_bus*(this_c->rank_id): config->t_bus * (config->rank_count - 1));
		 this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time +
		 config->t_amb_up+
		 (this_c->posted_cas == TRUE ? config->row_command_duration: 0);
		 this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time +
		 config->col_command_duration ;
		 // To calculate dimm data transmission time we need to check t_cwd
		 if (config->t_cwd >= 0) {
		 if (config->t_cwd > 0) {
		 this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time + config->t_cwd +
		 (config->posted_cas_flag ? config->t_al : 0);
		 this_c->dimm_data_tran_comp_time = this_c->dram_proc_comp_time + config->t_burst;
		 }else {
		 this_c->dram_proc_comp_time = 0;
		 this_c->dimm_data_tran_comp_time = this_c->dimm_comm_tran_comp_time + config->t_cwd + config->t_burst;
		 }
		 }else {
		 this_c->dram_proc_comp_time = 0;
		 this_c->dimm_data_tran_comp_time = this_c->amb_proc_comp_time + config->t_burst;
		 }
		 this_c->amb_down_proc_comp_time = 0;
		 this_c->link_data_tran_comp_time = 0;
		 this_c->rank_done_time = this_c->dimm_data_tran_comp_time;
		 rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;
		 rank[this_c->rank_id].my_dimm->data_bus_completion_time = this_c->dimm_data_tran_comp_time;
		 rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time = this_c->dimm_data_tran_comp_time + config->t_wr;
		 if (this_c->command == CAS_WRITE_AND_PRECHARGE) {
		 rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time =
		 MAX(  rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time, this_c->link_data_tran_comp_time + config->t_wr) + config->t_rp;
		 } else {
		 }
		 this_c->completion_time = this_c->rank_done_time;*/
	} else {
		set_col_command_bus(this_c->command, now);
		this_c->link_comm_tran_comp_time = now
				+ (config->posted_cas_flag ? config->col_command_duration : 0)
				+ config->col_command_duration;
		this_c->amb_proc_comp_time = 0;
		this_c->dimm_comm_tran_comp_time = 0;
		/** To calculate dimm data transmission time we need to check t_cwd */
		if (config->t_cwd >= 0) {
			if (config->t_cwd > 0) {
				this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time
						+ config->t_cwd
						+ (config->posted_cas_flag ? config->t_al : 0);
				this_c->link_data_tran_comp_time = this_c->dram_proc_comp_time
						+ config->t_burst;
			} else {
				this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time
						+ config->t_cwd
						+ (config->posted_cas_flag ? config->t_al : 0);
				this_c->link_data_tran_comp_time
						= this_c->link_comm_tran_comp_time + config->t_burst;
			}
		} else {
			this_c->dram_proc_comp_time = now;
			this_c->link_data_tran_comp_time = this_c->link_comm_tran_comp_time
					+ config->t_burst;
		}
		this_c->amb_down_proc_comp_time = 0;
		this_c->dimm_data_tran_comp_time = 0;
		this_c->rank_done_time = this_c->link_data_tran_comp_time;
		// Check if data bus time is being met at issue --- error checking DRAMsim
		data_bus.current_rank_id = MEM_STATE_INVALID;
		data_bus.completion_time = now
				+ (this_c->posted_cas ? config->col_command_duration : 0)
				+ config->col_command_duration + config->t_cwd
				+ (config->posted_cas_flag ? config->t_al : 0)
				+ config->t_burst;

		if ((config->t_cwd + config->col_command_duration) == 0) { /* no write delay */
			data_bus.status = BUS_BUSY;
			data_bus.completion_time = now + config->t_burst;
			rank[this_c->rank_id].bank[this_c->bank_id].cas_done_time = now
					+ config->t_burst;
		}
		rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time
				= this_c->link_data_tran_comp_time + config->t_wr;
		this_c->rank_done_time = this_c->link_data_tran_comp_time;
		if (this_c->command == CAS_WRITE_AND_PRECHARGE) {
			rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time
					= MAX( rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time, this_c->link_data_tran_comp_time + config->t_wr)
							+ config->t_rp;
		} else {
		}

		this_c->completion_time = this_c->link_data_tran_comp_time;
	}
	this_c->status = LINK_COMMAND_TRANSMISSION;
}

/*** FIXME : fbd - check that ras timing is met as far as bank goes rcd/ras
 * difference ***/
void Memory_Controller::issue_ras(tick_t now, Command *this_c) {
	int tran_index;
	/** Check as refresh commands do not map onto transaction queue **/
	this_c->status = LINK_COMMAND_TRANSMISSION;
	if (config->dram_type == FBD_DDR2) {
		/*this_c->link_comm_tran_comp_time = now +
		 config->t_bundle +
		 (config->var_latency_flag ?config->t_bus*(this_c->rank_id): config->t_bus * (config->rank_count - 1));
		 this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time +
		 config->t_amb_up;
		 this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time +
		 config->row_command_duration;
		 this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time +
		 config->t_rcd;
		 this_c->dimm_data_tran_comp_time = 0;
		 this_c->amb_down_proc_comp_time = 0;
		 this_c->link_data_tran_comp_time = 0;
		 this_c->rank_done_time = this_c->dimm_comm_tran_comp_time +   config->row_command_duration +  config->t_ras;
		 this_c->completion_time = this_c->dram_proc_comp_time;
		 if (this_c->command == RAS) {
		 rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time = this_c->amb_proc_comp_time + config->t_ras;
		 rank[this_c->rank_id].bank[this_c->bank_id].row_id = this_c->row_id;
		 rank[this_c->rank_id].bank[this_c->bank_id].rcd_done_time = this_c->amb_proc_comp_time + config->t_rcd;
		 rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;

		 }else { // RAS_ALL
		 int i,j;
		 for (i=0;i<config->rank_count;i++) {
		 rank[i].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;
		 if (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
		 for (j=0;j<config->bank_count;j++) {
		 rank[i].bank[j].ras_done_time =   this_c->amb_proc_comp_time +
		 config->t_ras;
		 rank[i].bank[j].rcd_done_time =   this_c->dram_proc_comp_time;
		 rank[i].bank[j].row_id =   this_c->row_id;
		 }
		 }else if  (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
		 rank[i].bank[this_c->bank_id].ras_done_time =   this_c->dram_proc_comp_time;
		 rank[i].bank[this_c->bank_id].row_id =   this_c->row_id;
		 }
		 }

		 }*/

	} else {
		set_row_command_bus(this_c->command, now);

		this_c->link_comm_tran_comp_time = now + config->row_command_duration;
		this_c->amb_proc_comp_time = 0;
		this_c->dimm_comm_tran_comp_time = 0;
		this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time
				+ config->t_rcd;
		this_c->dimm_data_tran_comp_time = 0;
		this_c->amb_down_proc_comp_time = 0;
		this_c->link_data_tran_comp_time = 0;
		this_c->rank_done_time = now + config->t_ras;
		tran_index = transaction_queue->get_transaction_index(this_c->tran_id);
		transaction_queue->entry[tran_index].status = MEM_STATE_ISSUED;
		rank[this_c->rank_id].bank[this_c->bank_id].row_id = this_c->row_id;
		rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time
				= this_c->rank_done_time;
		rank[this_c->rank_id].bank[this_c->bank_id].rcd_done_time
				= this_c->dram_proc_comp_time;
		this_c->completion_time = this_c->dram_proc_comp_time;

	}

	/* keep track of activation history for tFAW and tRRD */
	{
		int i;
		for (i = 3; i > 0; i--) {
			rank[this_c->rank_id].activation_history[i]
					= rank[this_c->rank_id].activation_history[i - 1];
		}
		rank[this_c->rank_id].activation_history[0] = now;
	}

}

void Memory_Controller::issue_prec(tick_t now, Command *this_c) {
	/** Check as refresh commands do not map onto transaction queue **/
	this_c->status = LINK_COMMAND_TRANSMISSION;

	if (config->dram_type == FBD_DDR2) {
		/**this_c->link_comm_tran_comp_time = now + config->t_bundle +
		 (config->var_latency_flag ?config->t_bus*(this_c->rank_id): config->t_bus * (config->rank_count - 1));
		 this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + config->t_amb_up;
		 this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time + config->row_command_duration;
		 this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time + config->t_rp;
		 this_c->dimm_data_tran_comp_time = 0;
		 this_c->amb_down_proc_comp_time = 0;
		 this_c->link_data_tran_comp_time = 0;
		 this_c->rank_done_time = this_c->dimm_comm_tran_comp_time;
		 this_c->completion_time = this_c->dram_proc_comp_time;

		 if (this_c->command == PRECHARGE) {
		 rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time = this_c->dram_proc_comp_time;
		 rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;
		 }else { // PRECHARGE_ALL
		 int i,j;
		 if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
		 for (j=0;j<config->bank_count;j++) {
		 rank[this_c->rank_id].bank[j].rp_done_time =  this_c->dram_proc_comp_time;
		 }
		 }else {
		 for (i=0;i<config->rank_count;i++) {
		 rank[i].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;
		 if (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
		 for (j=0;j<config->bank_count;j++) {
		 rank[i].bank[j].rp_done_time =  this_c->dram_proc_comp_time;
		 }
		 }else if  (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
		 rank[i].bank[this_c->bank_id].rp_done_time =
		 this_c->dram_proc_comp_time;
		 }
		 }
		 }
		 }**/

	} else {

		set_row_command_bus(this_c->command, now);
		this_c->link_comm_tran_comp_time = now + config->row_command_duration;
		this_c->amb_proc_comp_time = 0;
		this_c->dimm_comm_tran_comp_time = 0;
		this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time
				+ config->t_rp;
		this_c->dimm_data_tran_comp_time = 0;
		this_c->amb_down_proc_comp_time = 0;
		this_c->link_data_tran_comp_time = 0;
		this_c->rank_done_time = this_c->dram_proc_comp_time;
		this_c->completion_time = this_c->dram_proc_comp_time;

		if (this_c->command == PRECHARGE) {
			rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time
					= this_c->dram_proc_comp_time;
		} else { /** PRECHARGE_ALL **/
			int i, j;
			if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
				for (j = 0; j < config->bank_count; j++) {
					rank[this_c->rank_id].bank[j].rp_done_time
							= this_c->dram_proc_comp_time;
				}
			} else {
				for (i = 0; i < config->rank_count; i++) {
					if (config->refresh_policy
							== REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
						for (j = 0; j < config->bank_count; j++) {
							rank[i].bank[j].rp_done_time
									= this_c->dram_proc_comp_time;
						}
					} else if (config->refresh_policy
							== REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
						rank[i].bank[this_c->bank_id].rp_done_time
								= this_c->dram_proc_comp_time;
					}
				}
			}
		}

	}
}

void Memory_Controller::issue_ras_or_prec(tick_t now, Command *this_c) {
	int tran_index;
	tran_index = transaction_queue->get_transaction_index(this_c->tran_id);
	transaction_queue->entry[tran_index].status = MEM_STATE_ISSUED;
	set_row_command_bus(this_c->command, now);
	this_c->status = LINK_COMMAND_TRANSMISSION;
}

void Memory_Controller::issue_data(tick_t now, Command *this_c) {
	//  int tran_index;
	//  tran_index = get_transaction_index(this_c->tran_id);
	if (config->dram_type != FBD_DDR2) {
		fprintf(stdout, "are we even doing this?");
	} else {
		/**this_c->status = LINK_COMMAND_TRANSMISSION;
		 lock_buffer(this_c);

		 this_c->link_comm_tran_comp_time = now + config->t_bundle +
		 (config->var_latency_flag ?config->t_bus*(this_c->rank_id): config->t_bus * (config->rank_count - 1));
		 this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + config->t_amb_up;
		 this_c->dimm_comm_tran_comp_time = 0;
		 this_c->dram_proc_comp_time = 0;
		 this_c->dimm_data_tran_comp_time = 0;
		 this_c->amb_down_proc_comp_time = 0;
		 this_c->link_data_tran_comp_time = 0;
		 this_c->rank_done_time = this_c->amb_proc_comp_time;
		 this_c->completion_time = this_c->rank_done_time;
		 if (config->amb_buffer_debug())
		 print_buffer_contents( this_c->rank_id);**/

	}

}

void Memory_Controller::issue_drive(tick_t now, Command *this_c) {
	//  int tran_index;
	//  tran_index = get_transaction_index(this_c->tran_id);
	// transaction_queue->entry[tran_index].status = MEM_STATE_ISSUED;
	if (config->dram_type != FBD_DDR2) {
		fprintf(stdout, "are we even doing this?");
	} else {
		/**this_c->link_comm_tran_comp_time = now + config->t_bundle +
		 (config->var_latency_flag ?config->t_bus*(this_c->rank_id): config->t_bus * (config->rank_count - 1));
		 this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + config->t_amb_up;
		 this_c->dimm_comm_tran_comp_time = 0;
		 this_c->dram_proc_comp_time = 0;
		 this_c->dimm_data_tran_comp_time = 0;
		 this_c->amb_down_proc_comp_time = this_c->amb_proc_comp_time + config->t_amb_down;
		 this_c->link_data_tran_comp_time = this_c->amb_down_proc_comp_time + this_c->data_word * config->t_bundle;
		 this_c->rank_done_time = this_c->link_data_tran_comp_time;
		 down_bus.completion_time = this_c->amb_down_proc_comp_time +
		 config->t_bundle * this_c->data_word;
		 this_c->completion_time = this_c->rank_done_time;**/
	}
	this_c->status = LINK_COMMAND_TRANSMISSION;
}

void Memory_Controller::issue_refresh(tick_t now, Command *this_c) {
	this_c->status = LINK_COMMAND_TRANSMISSION;
	if (config->dram_type == FBD_DDR2) {
		/**this_c->link_comm_tran_comp_time = now +
		 config->t_bundle +
		 (config->var_latency_flag ?config->t_bus*(this_c->rank_id): config->t_bus * (config->rank_count - 1));
		 this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time +
		 config->t_amb_up;
		 this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time +
		 config->row_command_duration;
		 this_c->dimm_data_tran_comp_time = 0;
		 this_c->amb_down_proc_comp_time = 0;
		 this_c->link_data_tran_comp_time = 0;

		 if (this_c->command == REFRESH) { //Single bank refresh
		 this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time +
		 config->t_rc;
		 this_c->rank_done_time = this_c->dram_proc_comp_time;
		 this_c->completion_time = this_c->rank_done_time;
		 rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time = this_c->dram_proc_comp_time;
		 //	  rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time = this_c->dram_proc_comp_time;
		 rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;

		 }else { // REFRESH_ALL
		 this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time +
		 config->t_rfc;
		 this_c->rank_done_time = this_c->dram_proc_comp_time;
		 this_c->completion_time = this_c->rank_done_time;
		 int i,j;
		 if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
		 rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;
		 for (j=0;j<config->bank_count;j++) {
		 rank[this_c->rank_id].bank[j].rp_done_time =   this_c->dram_proc_comp_time;
		 }
		 }else {
		 for (i=0;i<config->rank_count;i++) {
		 rank[i].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;
		 if (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
		 for (j=0;j<config->bank_count;j++) {
		 rank[i].bank[j].rp_done_time =   this_c->dram_proc_comp_time;
		 }
		 }else if  (config->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
		 rank[i].bank[this_c->bank_id].rp_done_time =   this_c->dram_proc_comp_time;
		 }
		 }
		 }

		 }**/

	} else {
		set_row_command_bus(this_c->command, now);

		this_c->link_comm_tran_comp_time = now + config->row_command_duration;
		this_c->amb_proc_comp_time = 0;
		this_c->dimm_comm_tran_comp_time = 0;
		this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time
				+ config->t_rc;
		this_c->dimm_data_tran_comp_time = 0;
		this_c->amb_down_proc_comp_time = 0;
		this_c->link_data_tran_comp_time = 0;

		this_c->rank_done_time = this_c->dram_proc_comp_time;
		if (this_c->command == REFRESH) {
			rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time
					= this_c->dram_proc_comp_time;

		} else { /** REFRESH_ALL **/
			if (config->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
				int j;
				for (j = 0; j < config->bank_count; j++) {
					rank[this_c->rank_id].bank[j].rp_done_time
							= this_c->dram_proc_comp_time;
				}
			} else {

				int i, j;
				for (i = 0; i < config->rank_count; i++) {
					if (config->refresh_policy
							== REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
						for (j = 0; j < config->bank_count; j++) {
							rank[i].bank[j].rp_done_time
									= this_c->dram_proc_comp_time;
						}
					} else if (config->refresh_policy
							== REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
						rank[i].bank[this_c->bank_id].rp_done_time
								= this_c->dram_proc_comp_time;
					}
				}
			}

		}
	}
}

void Memory_Controller::update_critical_word_info(tick_t now, Command * this_c,
		Transaction *this_t) {
	if (config->critical_word_first_flag == TRUE) {
		if (config->dram_type == FBD_DDR2)
			this_t->critical_word_ready_time = this_c->amb_down_proc_comp_time
					+ config->t_bundle + config->tq_delay;
		else
			this_t->critical_word_ready_time = this_c->dram_proc_comp_time
					+ config->dram_clock_granularity + config->tq_delay;
	} else {
		this_t->critical_word_ready_time = this_c->link_data_tran_comp_time
				+ config->tq_delay;
	}

}

/**
 * Generic Issue Command
 */
void Memory_Controller::issue_cmd(tick_t now, Command *this_c,
		Transaction *this_t) {

	//assert(this_c != NULL);
	//assert(this_c->status == IN_QUEUE);
	switch (this_c->command) {
	case RAS:
		issue_ras(now, this_c);
		break;
	case CAS_AND_PRECHARGE:
	case CAS:
		issue_cas(now, this_c);
		break;
		///case CAS_WITH_DRIVE : issue_cas_with_drive (now,this_c);
		///					  break;
	case CAS_WRITE_AND_PRECHARGE:
	case CAS_WRITE:
		issue_cas_write(now, this_c);
		break;
	case PRECHARGE:
		issue_prec(now, this_c);
		break;
	case RAS_ALL:
		issue_ras(now, this_c);
		break;
	case PRECHARGE_ALL:
		issue_prec(now, this_c);
		break;
	case REFRESH_ALL:
		issue_refresh(now, this_c);
		break;
	case REFRESH:
		issue_refresh(now, this_c);
		break;
	}
	if (rank[this_c->rank_id].cke_bit == false
			|| (rank[this_c->rank_id].cke_bit == true
					&& rank[this_c->rank_id].cke_done < this_c->rank_done_time)) {
		rank[this_c->rank_id].cke_bit = true;
		rank[this_c->rank_id].cke_tid = this_c->tran_id;
		rank[this_c->rank_id].cke_cmd = this_c->command;
		rank[this_c->rank_id].cke_done = this_c->rank_done_time;
#ifdef DEBUG_POWER
		fprintf(stdout," [%llu] CKE Bit r[%d] c[%d] t[%d] Till[%llu] ",now,
				this_c->rank_id,
				chan_id,
				this_c->tran_id,
				this_c->rank_done_time);
#endif
	}
	if (!this_c->refresh)
		this_t->status = MEM_STATE_ISSUED;
	this_c->start_transmission_time = now;
	//if (this_c->command == CAS || this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_WRITE)
	//transaction_queue->entry[get_transaction_index(this_c->tran_id)].issue_cas = true;

	if (config->get_tran_watch(this_c->tran_id))
		this_c->print_command(now);
	// Do the power update
	update_power_stats(now, this_c);
	if (this_c->command == CAS_AND_PRECHARGE || this_c->command == CAS
			|| this_c->command == CAS_WITH_DRIVE) {
		int tran_index = transaction_queue->get_transaction_index(
				this_c->tran_id);
		update_critical_word_info(now, this_c,
				&transaction_queue->entry[tran_index]);
	}
	if (this_c->command == CAS || this_c->command == CAS_WITH_DRIVE
			|| this_c->command == CAS_WRITE || this_c->command
			== CAS_AND_PRECHARGE || this_c->command == CAS_WRITE_AND_PRECHARGE) {
		//print_command_detail(now,this_c);
		this_t->issued_cas = true;
		rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras++;
	}
	if (this_c->command == RAS) {
		this_t->issued_ras = true;
		if (rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras
				!= 0)
			aux_stat->mem_gather_stat(
					GATHER_BANK_CONFLICT_STAT,
					(int) (rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras));
		rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras = 0;

	}
	if ((config->issue_debug_flag && (config->tq_info.transaction_id_ctr
			>= config->tq_info.debug_tran_id_threshold))) {
		if (this_c->posted_cas)
			this_c->print_command_detail(now + config->row_command_duration);
		else
			this_c->print_command_detail(now);
	}
}

// Note that only r_p_info needs to make sure that power for a given interval
// is not over-calculated.
// r_p_gblinfo needs to make sure that power overall is calculated okay
//
void Memory_Controller::update_power_stats(tick_t now, Command *this_c) {

	int chan_id = this_c->chan_id;
	int rank_id = this_c->rank_id;
	tick_t CAS_carry_cycle = 0;
	tick_t CAW_carry_cycle = 0;
	DRAM_config *pConfig = config;

	if (this_c->command == CAS || this_c->command == CAS_WITH_DRIVE
			|| this_c->command == CAS_AND_PRECHARGE) {
		rank[rank_id].r_p_info.current_CAS_cycle += pConfig->t_burst;
		rank[rank_id].r_p_gblinfo.sum_per_RD += pConfig->t_burst;
#ifdef DEBUG_POWER
		fprintf(stdout,"\t+++++rank[%d-%d] >>CAS_cycle[%llu] CAW_cycle[%llu] ",
				chan_id,rank_id,
				rank[rank_id].r_p_info.current_CAS_cycle,
				rank[rank_id].r_p_info.current_CAW_cycle);
		fprintf(stdout,"last_RAS[%llu]\n",
				(tick_t) rank[rank_id].r_p_info.last_RAS_time);
#endif
		//Ohm--stat for dram_access
		rank[rank_id].r_p_info.dram_access++;
		rank[rank_id].r_p_gblinfo.dram_access++;
		rank[rank_id].r_p_info.last_CAS_time = now;
		rank[rank_id].r_p_gblinfo.last_CAS_time = now;
	}
	if (this_c->command == CAS_WRITE || this_c->command
			== CAS_WRITE_AND_PRECHARGE) {
		rank[rank_id].r_p_info.current_CAW_cycle += pConfig->t_burst;
		rank[rank_id].r_p_gblinfo.sum_per_WR += pConfig->t_burst;
#ifdef DEBUG_POWER
		fprintf(stdout,"\tXXXXrank[%d-%d] >>CAS_cycle[%llu] CAW_cycle[%llu] ",
				chan_id,rank_id,
				rank[rank_id].r_p_info.current_CAS_cycle,
				rank[rank_id].r_p_info.current_CAW_cycle);
		fprintf(stdout,"last_RAS[%llu]\n",
				(tick_t) rank[rank_id].r_p_gblinfo.last_RAS_time);
#endif
		//Ohm--stat for dram_access
		rank[rank_id].r_p_info.dram_access++;
		rank[rank_id].r_p_gblinfo.dram_access++;
		rank[rank_id].r_p_info.last_CAW_time = now;
		rank[rank_id].r_p_gblinfo.last_CAW_time = now;
	}
	if (this_c->command == RAS) {
		/* Ohm--put code to track t_ACT HERE */
		rank[rank_id].r_p_info.RAS_count++;
		if (rank[rank_id].r_p_info.last_RAS_time == 0) {
			//At the beginning of the printing period
			rank[rank_id].r_p_info.last_RAS_time = now;
			rank[rank_id].r_p_info.RAS_count--;
		}
		rank[rank_id].r_p_info.RAS_sum += (now
				- rank[rank_id].r_p_info.last_RAS_time);

		//Ohm: change how to calculate RD% and WR%
		if (rank[rank_id].r_p_info.current_CAS_cycle > 0) {
			if (rank[rank_id].r_p_info.current_CAS_cycle > now
					- rank[rank_id].r_p_info.last_RAS_time) {
				rank[rank_id].r_p_info.sum_per_RD += now
						- rank[rank_id].r_p_info.last_RAS_time;
				CAS_carry_cycle = rank[rank_id].r_p_info.current_CAS_cycle
						- (now - rank[rank_id].r_p_info.last_RAS_time);
			} else
				rank[rank_id].r_p_info.sum_per_RD
						+= rank[rank_id].r_p_info.current_CAS_cycle;
		}
		if (rank[rank_id].r_p_info.current_CAW_cycle > 0) {
			if (rank[rank_id].r_p_info.current_CAW_cycle > now
					- rank[rank_id].r_p_info.last_RAS_time) {
				rank[rank_id].r_p_info.sum_per_WR += now
						- rank[rank_id].r_p_info.last_RAS_time;
				CAW_carry_cycle = rank[rank_id].r_p_info.current_CAW_cycle
						- (now - rank[rank_id].r_p_info.last_RAS_time);
			} else
				rank[rank_id].r_p_info.sum_per_WR
						+= rank[rank_id].r_p_info.current_CAW_cycle;
		}
		rank[rank_id].r_p_info.current_CAS_cycle = CAS_carry_cycle;
		rank[rank_id].r_p_info.current_CAW_cycle = CAW_carry_cycle;
		rank[rank_id].r_p_info.last_RAS_time = now;
#ifdef DEBUG_POWER
		fprintf(stdout,"\t***RAS issued @rank[%d-%d] CAS_cycle[%llu] CAW_cycle[%llu] ",
				chan_id, rank_id,
				rank[rank_id].r_p_info.current_CAS_cycle,
				rank[rank_id].r_p_info.current_CAW_cycle);
		fprintf(stdout, "RAS_count[%llu] RAS_sum[%llu]",
				rank[rank_id].r_p_info.RAS_count,
				rank[rank_id].r_p_info.RAS_sum);
		fprintf(stdout, "sum_RD[%llu] sum_WR[%llu]\n",
				rank[rank_id].r_p_info.sum_per_RD,
				rank[rank_id].r_p_info.sum_per_WR);
#endif

		rank[rank_id].r_p_gblinfo.RAS_count++;
		if (rank[rank_id].r_p_gblinfo.last_RAS_time == 0) {
			//At the beginning
			rank[rank_id].r_p_gblinfo.last_RAS_time = now;
			rank[rank_id].r_p_gblinfo.RAS_count--;
		}

		rank[rank_id].r_p_gblinfo.RAS_sum += (now
				- rank[rank_id].r_p_gblinfo.last_RAS_time);
		rank[rank_id].r_p_gblinfo.last_RAS_time = now;

	}
	// How does one do REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK power
	// and REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK
	if (this_c->command == REFRESH_ALL) {
		if (pConfig->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
			rank[rank_id].r_p_gblinfo.refresh_count++;
			rank[rank_id].r_p_info.refresh_count++;
		} else if (pConfig->refresh_policy
				== REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
			int i;
			for (i = 0; i < pConfig->rank_count; i++) {
				rank[i].r_p_gblinfo.refresh_count++;
				rank[i].r_p_info.refresh_count++;
			}
		}
	}
}

