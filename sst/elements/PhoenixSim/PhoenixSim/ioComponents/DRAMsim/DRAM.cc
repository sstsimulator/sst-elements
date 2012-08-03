#include "DRAM.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "utilities.h"

//tick_t DRAM::current_dram_time;

DRAM::DRAM() {

	config = new DRAM_config();

	///biu = new BIU(config);

	init_dram_system();

	power_config = new PowerConfig(config, dram_controller);

	config->dram_power_config = power_config;

}

DRAM::DRAM(BIU* b, DRAM_config* cfg, Aux_Stat* st) {

	aux_stat = st;
	config = cfg;

	///biu = b;
	init_dram_system();

	power_config = new PowerConfig(cfg, dram_controller);

	config->dram_power_config = power_config;
}

DRAM_config* DRAM::get_dram_system_config() {
	return config;
}

tick_t DRAM::get_dram_current_time() {
	return current_dram_time;
}

/*
 *   Functions associated with memory controllers.
 */

/*
 *  This is the interface to the entire memory system.
 *  The memory system takes commands from biubufferptr, and
 *  returns results to the biubufferptr as well.
 *  This update needs the selection policy here
 */

void DRAM::init_dram_system() {
	int i;
	last_dram_time = 0;
	current_dram_time = 0;

	dram_controller = (Memory_Controller**) malloc(sizeof(Memory_Controller)
			* MAX_CHANNEL_COUNT);
	for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
		dram_controller[i] = new Memory_Controller(config, aux_stat, i);
	}
}

void DRAM::print_transaction_queue() {

	Command *this_c;
	int i;
	int chan_id;
	for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
		for (i = 0; i < MAX_TRANSACTION_QUEUE_DEPTH; i++) {
			fprintf(
					stdout,"Entry[%2d] Status[%2d] Start_time[%8d] ID[%u] Access Type[%2d] Addr[0x%8X] Channel[%2d] \n",
					i,
					dram_controller[chan_id]->transaction_queue->entry[i].status,
					(int) dram_controller[chan_id]->transaction_queue->entry[i].arrival_time,
					dram_controller[chan_id]->transaction_queue->entry[i].transaction_id,
					dram_controller[chan_id]->transaction_queue->entry[i].transaction_type,
					dram_controller[chan_id]->transaction_queue->entry[i].address.physical_address,
					dram_controller[chan_id]->transaction_queue->entry[i].address.chan_id);
			this_c
					= dram_controller[chan_id]->transaction_queue->entry[i].next_c;
			while (this_c != NULL) {
				this_c->print_command(current_dram_time);
				this_c = this_c->next_c;
			}
		}
	}
}

int DRAM::update_dram(tick_t current_cpu_time) { /* This code should work for SDRAM, DDR SDRAM as well as RDRAM */

	//fprintf(stdout, "start update dram\n");
	tick_t dram_stop_time;
	tick_t expected_dram_time;
	tick_t missing_dram_time;
	int i;
	int done;
	int chan_id;
	char debug_string[MAX_DEBUG_STRING_SIZE];
	//Ohm--Add for power calculation
	int all_bnk_pre;
	int j;
	int precharge_bank = 0;

	int outstanding = 0;

	expected_dram_time = (tick_t) (config->mem2cpu_clock_ratio
			* current_cpu_time);
	dram_stop_time = (tick_t) (config->mem2cpu_clock_ratio * (current_cpu_time
			+ 1)); /* 1 cpu tick into the future */
	debug_string[0] = '\0';

	//fprintf(stdout, "1\n");

	if ((expected_dram_time - last_dram_time) < config->dram_clock_granularity) { /* handles DDR, QDR and everything else. */
		//fprintf(stdout, "returning here: mem2cpu = %f, clock_gran = %d, expected = %d, current_dram = %d, current_cpu = %d, last = %d\n", config->mem2cpu_clock_ratio, config->dram_clock_granularity, expected_dram_time, current_dram_time, current_cpu_time, last_dram_time);
		//fprintf(stdout, "end update dram\n");
		return 1;
	} else if ((config->auto_refresh_enabled == TRUE)) {
		for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
			dram_controller[chan_id]->update_refresh_missing_cycles(
					current_dram_time, debug_string);
		}
	}
	missing_dram_time = expected_dram_time - (last_dram_time
			+ config->dram_clock_granularity);//OHm: work here !!!!
	if (missing_dram_time > 0) {
		//fprintf(stdout, "missing time %d\n", missing_dram_time);
		//decide if the missing cycles are for ACTIVE or PRECHARGE state
		//fprintf(stdout, "2\n");
		for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
			for (i = 0; i < config->rank_count; i++) {
				for (j = 0; j < config->bank_count; j++) {
					if ((dram_controller[chan_id]->rank[i].bank[j].last_command
							== PRECHARGE)
							|| (dram_controller[chan_id]->rank[i].bank[j].status
									== PRECHARGING)) {
						precharge_bank++;
					}
				}
				if (precharge_bank == config->bank_count) {//all bank precharged
#ifdef DEBUG_POWER
					fprintf(stdout,"all_bnk_pre:missing_cycles[%d]",missing_dram_time);
#endif
					dram_controller[chan_id]->rank[i].r_p_info.bnk_pre
							+= missing_dram_time;
					dram_controller[chan_id]->rank[i].r_p_gblinfo.bnk_pre
							+= missing_dram_time;
				}
			}
		}
	}
	//fprintf(stdout, "3\n");
	current_dram_time = expected_dram_time;
	//fprintf(stdout, "dram time: %d\n", current_dram_time);
	//fprintf(stdout, "current_dram_time %d, dram_stop_time\n", missing_dram_time);
	while (current_dram_time <= dram_stop_time) { /* continue to simulate until time limit */
		/* First thing we have to do is to figure out if a certain command has completed */
		/* Then if that command has completed, we can then retire the transaction */
		debug_string[0] = '\0';
		for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
			for (i = 0; i
					< dram_controller[chan_id]->transaction_queue->transaction_count; i++) {
				Transaction
						*trans =
								&(dram_controller[chan_id]->transaction_queue->entry[i]);
				if (trans->status != MEM_STATE_COMPLETED && trans->status
						!= MEM_STATE_SCHEDULED) {
					done = dram_controller[chan_id]->update_command_states(
							current_dram_time, chan_id, i, debug_string); /** Sending transaction index **/
					if (done == TRUE) {
#ifdef DEBUG
						if (config->tq_info.get_transaction_debug()) {
							fprintf(
									stdout,"Completed @ [%llu] Duration [%llu] :",
									current_dram_time,
									current_dram_time,
									dram_controller[chan_id]->transaction_queue->entry[i].arrival_time);
							///trans->print_transaction_index(current_dram_time, biu->get_rid(trans->slot_id));
						}
#endif
						trans->completion_time = current_dram_time
								+ config->tq_delay;
						trans->status = MEM_STATE_COMPLETED;

					}
				}
			}
		}

		//fprintf(stdout, "start retire\n");
		retire_transactions();
		//fprintf(stdout, "end retire\n");

		/* now look through the current transaction list for commands we can schedule. *If* the command bus is free */
		/* AND that command has to be able to be issued, subject to DRAM system restrictions */
#ifdef DEBUG
		if (config->wave_debug() || config->cas_wave_debug()) { /* draw cheesy text waveform output */
			fprintf(stdout,"%8d  ", (int) current_dram_time);
		}
#endif

		//fprintf(stdout, "4\n");
		gather_tran_stats();
		gather_biu_slot_stats();

		//fprintf(stdout, "5\n");

		for (chan_id = 0; chan_id < config->channel_count; chan_id++) {

			dram_controller[chan_id]->update_refresh_status(current_dram_time,
					debug_string);
			dram_controller[chan_id]->update_bus_link_status(current_dram_time); /* added to take t_bus out of consideration when changing bus status */

			outstanding
					+= dram_controller[chan_id]->transaction_queue->transaction_count;

			if (config->dram_type != FBD_DDR2) {
				dram_controller[chan_id]->issue_new_commands(current_dram_time,
						debug_string);
			} else {
				// Schedule only on even clock cycles
				///if (current_dram_time%2 == 0 ) {
				///  	commands2bundle(current_dram_time, chan_id,debug_string);
				///}
			}
			// You can only do this once
			///schedule_new_transaction(chan_id);

			if (config->auto_refresh_enabled == TRUE) {
				dram_controller[chan_id]->insert_refresh_transaction(
						current_dram_time);
			}

			dram_controller[chan_id]->update_cke_bit(current_dram_time,
					config->rank_count);
#ifdef DEBUG
			if (config->wave_debug() || config->cas_wave_debug()) { /* draw cheesy text waveform output */
				if (config->dram_type == FBD_DDR2) {
					/*** FIXME We need to figure out how to draw our waveform **/
					/**fprintf(stdout," CH[%d] ",chan_id);
					 if(dram_controller[chan_id]->up_bus.status == IDLE){
					 fprintf(stdout," UI ");
					 } else {
					 fprintf(stdout," UD ");
					 }
					 if(dram_controller[chan_id]->down_bus.status == IDLE){
					 fprintf(stdout," DI ");
					 } else {
					 fprintf(stdout," DD ");
					 }
					 fprintf(stdout,"%s",debug_string);
					 fprintf(stdout,"\n");**/
				} else {
					if (dram_controller[chan_id]->data_bus.status == IDLE) {
						fprintf(stdout," I ");
					} else {
						fprintf(stdout," D ");
					}
					if (dram_controller[chan_id]->col_command_bus_idle()) { /* unified command bus or just row command bus */
						fprintf(stdout," |   ");
					} else {
						fprintf(stdout,"   | ");
					}
					fprintf(stdout,"%s", debug_string);
				}
			} else if (config->dram_debug()) {
				fprintf(stdout,"%s", debug_string);

			}
			debug_string[0] = '\0';
#endif
		}

		//fprintf(stdout, "6\n");

		/* Ohm--add the code to track BNK_PRE% HERE  */

		for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
			for (i = 0; i < config->rank_count; i++) {
				all_bnk_pre = TRUE;
				for (j = 0; j < config->bank_count; j++) {
					if (dram_controller[chan_id]->rank[i].bank[j].rp_done_time
							< dram_controller[chan_id]->rank[i].bank[j].ras_done_time)
						all_bnk_pre = FALSE;
				}

				if (all_bnk_pre == TRUE) {
#ifdef DEBUG_POWER
					fprintf(stdout,"[%d]all_bnk_pre:",i);
#endif
					dram_controller[chan_id]->rank[i].r_p_info.bnk_pre
							+= config->dram_clock_granularity;
					dram_controller[chan_id]->rank[i].r_p_gblinfo.bnk_pre
							+= config->dram_clock_granularity;
					if (dram_controller[chan_id]->check_cke_hi_pre(i)) {
						dram_controller[chan_id]->rank[i].r_p_info.cke_hi_pre
								+= config->dram_clock_granularity;
						dram_controller[chan_id]->rank[i].r_p_gblinfo.cke_hi_pre
								+= config->dram_clock_granularity;
					} else {
#ifdef DEBUG_POWER
						fprintf(stdout,"[%d]cke_lo_pre",i);
#endif
					}
				} else //ACTIVE state
				{
					//to find out CKE_HI_ACT
					if (dram_controller[chan_id]->check_cke_hi_act(i)) {
						dram_controller[chan_id]->rank[i].r_p_info.cke_hi_act
								+= config->dram_clock_granularity;;
						dram_controller[chan_id]->rank[i].r_p_gblinfo.cke_hi_act
								+= config->dram_clock_granularity;;
					} else {
#ifdef DEBUG_POWER
						fprintf(stdout,"[%d]cke_lo_act",i);
#endif
					}
				}
			}
		}
		//fprintf(stdout, "7\n");
		/* Ohm--Then, print me the power stats */
		if ((config->dram_type == DDRSDRAM) || (config->dram_type == DDR2)
				|| (config->dram_type == FBD_DDR2) || (config->dram_type == DDR3)) {
			//power_config->print_power_stats(current_dram_time);
		}

		last_dram_time = current_dram_time;
		if (current_dram_time % 2 == 0)
			current_dram_time += config->dram_clock_granularity;
		else
			current_dram_time += 1;
#ifdef DEBUG
		if (config->wave_debug() || config->dram_debug()
				|| config->cas_wave_debug()) {
			fprintf(stdout,"\n");
		}
		assert (strlen(debug_string) < MAX_DEBUG_STRING_SIZE);
#endif

	}

	//fprintf(stdout, "end update dram\n");

	//fprintf(stdout, "update finished\n");
	return outstanding;

}

bool DRAM::is_dram_busy() {
	bool busy = false;

	int chan_id;

	for (chan_id = 0; busy == false && chan_id < config->channel_count; chan_id++) {
		if (dram_controller[chan_id]->transaction_queue->transaction_count > 0)
			busy = true;
		busy = dram_controller[chan_id]->is_refresh_pending();
	}
	return busy;
}

void DRAM::retire_transactions() {

	/* retire the transaction, and return the status to the bus queue slot */
	/* The out of order completion of the transaction is possible because the later transaction may
	 hit an open page, where as the earlier transaction may be delayed by a PRECHARGE + RAS or just RAS.
	 Check to see if transaction ordering is strictly enforced.

	 This option is current disabled as of 9/20/2002.  Problem with Mase generation non-unique req_id's.
	 out of order data return creates havoc there. This should be investigated and fixed.
	 */
	int rank_id, bank_id, row_id, tran_id;
	int i;
	Transaction * this_t = NULL;
	int chan_id;
	if (config->strict_ordering_flag == TRUE) { /* if strict ordering is set, check only transaction #0 */
		for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
			this_t = &dram_controller[chan_id]->transaction_queue->entry[0];
			chan_id = this_t->next_c->chan_id;
			if ((this_t->status == MEM_STATE_COMPLETED)
					&& (this_t->completion_time <= current_dram_time)) {
				/* If this is a refresh or auto precharge transaction, just ignore it. Else... */

				///	biu->set_critical_word_ready(this_t->slot_id);

				///	biu->set_biu_slot_status(this_t->slot_id, MEM_STATE_COMPLETED);
				int trans_sel_policy =
						config->get_transaction_selection_policy();
				if (trans_sel_policy == MOST_PENDING || trans_sel_policy
						== LEAST_PENDING)
					rank_id
							= dram_controller[chan_id]->transaction_queue->entry[0].address.rank_id;
				bank_id
						= dram_controller[chan_id]->transaction_queue->entry[0].address.bank_id;
				row_id
						= dram_controller[chan_id]->transaction_queue->entry[0].address.row_id;
				tran_id
						= dram_controller[chan_id]->transaction_queue->entry[0].transaction_id;
				dram_controller[chan_id]->pending_queue.del_trans_pending_queue(
						tran_id, rank_id, bank_id, row_id);

				/*---------------------small stat counter----------------------*/
				switch (this_t->transaction_type) {
				case (MEMORY_IFETCH_COMMAND):
					config->tq_info.num_ifetch_tran++;
					break;
				case (MEMORY_READ_COMMAND):
					config->tq_info.num_read_tran++;
					break;
				case (MEMORY_PREFETCH):
					config->tq_info.num_prefetch_tran++;
					break;
				case (MEMORY_WRITE_COMMAND):
					config->tq_info.num_write_tran++;
					break;
				default:
					fprintf(stdout, "unknown transaction type %d\n",
							this_t->transaction_type);
					break;
				}
				/*---------------------end small stat counter----------------------*/
				// Do the update on bank conflict count and bank hit count
				if (!this_t->issued_ras && this_t->issued_cas) {
					dram_controller[chan_id]->rank[this_t->address.rank_id].bank[this_t->address.bank_id].bank_hit++;
				} else if (this_t->issued_ras && this_t->issued_cas) {
					dram_controller[chan_id]->rank[this_t->address.rank_id].bank[this_t->address.bank_id].bank_conflict++;
				}
				dram_controller[chan_id]->transaction_queue->remove_transaction(
						0, current_dram_time);
				int thread_id = this_t->thread_id;
				int trans_id = this_t->request_id;
				fprintf(
						stdout,
						" completed transaction %d, request_id %d, thread_id %d, count %d\n",
						this_t->transaction_id,
						trans_id,
						thread_id,
						dram_controller[chan_id]->transaction_queue->transaction_count);

				callback->return_transaction(thread_id, trans_id);

				///if (config->row_buffer_management_policy == OPEN_PAGE)
				///	update_tran_for_refresh(chan_id);
			} ///else if ((this_t->critical_word_ready == TRUE)
			///		&& (this_t->critical_word_ready_time <= current_dram_time)) {
			///	biu->set_critical_word_ready(this_t->slot_id);
			///}
		}
	} else {
		for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
			for (i = 0; i
					< dram_controller[chan_id]->transaction_queue->transaction_count; i++) {
				this_t = &dram_controller[chan_id]->transaction_queue->entry[i];
				chan_id = this_t->next_c->chan_id;
			//	fprintf(stdout, "status %d\n", this_t->status);
				if ((this_t->status == MEM_STATE_COMPLETED)
						&& (this_t->completion_time <= current_dram_time)) {
					///if ((biu->get_biu_slot_status(this_t->slot_id)
					///	!= MEM_STATE_COMPLETED)) {
					// If this is a refresh or auto precharge transaction, just ignore it. Else...
					//fprintf(stdout, "DRAM: completing transaction\n");
					///biu->set_critical_word_ready(this_t->slot_id);
					///biu->set_biu_slot_status(this_t->slot_id,
					///		MEM_STATE_COMPLETED);
					///}
					int trans_sel_policy =
							config->get_transaction_selection_policy();
					if (trans_sel_policy == MOST_PENDING || trans_sel_policy
							== LEAST_PENDING)
						rank_id
								= dram_controller[chan_id]->transaction_queue->entry[i].address.rank_id;
					bank_id
							= dram_controller[chan_id]->transaction_queue->entry[i].address.bank_id;
					row_id
							= dram_controller[chan_id]->transaction_queue->entry[i].address.row_id;
					tran_id
							= dram_controller[chan_id]->transaction_queue->entry[i].transaction_id;
					dram_controller[chan_id]->pending_queue.del_trans_pending_queue(
							tran_id, rank_id, bank_id, row_id);

					//---------------------small stat counter----------------------
					switch (this_t->transaction_type) {
					case (MEMORY_IFETCH_COMMAND):
						config->tq_info.num_ifetch_tran++;
						break;
					case (MEMORY_READ_COMMAND):
						config->tq_info.num_read_tran++;
						break;
					case (MEMORY_PREFETCH):
						config->tq_info.num_prefetch_tran++;
						break;
					case (MEMORY_WRITE_COMMAND):
						config->tq_info.num_write_tran++;
						break;
					default:
						fprintf(stdout, "unknown transaction type %d ID %d\n",
								this_t->transaction_type,
								this_t->transaction_id);
						break;
					}
					//---------------------end small stat counter----------------------

					// Do the update on bank conflict count and bank hit count
					if (!this_t->issued_ras && this_t->issued_cas) {
						dram_controller[chan_id]->rank[this_t->address.rank_id].bank[this_t->address.bank_id].bank_hit++;
					} else if (this_t->issued_ras && this_t->issued_cas) {
						dram_controller[chan_id]->rank[this_t->address.rank_id].bank[this_t->address.bank_id].bank_conflict++;
					}

					int thread_id = this_t->thread_id;
					int req_id = this_t->request_id;
					int trans_id = this_t->transaction_id;

					//std::cout << "my callback: " << callback << std::endl;

					callback->return_transaction(thread_id, req_id);
					//fprintf(stdout, " completed transaction %d, request_id %d, thread_id %d addr %d, count %d\n", trans_id, req_id, thread_id,  this_t->address.physical_address, dram_controller[chan_id]->transaction_queue->transaction_count );
					dram_controller[chan_id]->transaction_queue->remove_transaction(
							i, current_dram_time);

				} else if ((this_t->critical_word_ready == TRUE)
						&& (this_t->critical_word_ready_time
								<= current_dram_time)) {
					///  biu->set_critical_word_ready(this_t->slot_id);
				}
			}
		}
	}
}

void DRAM::gather_tran_stats() {
	int chan_id;
	int total_trans = 0;
	for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
		total_trans
				+= dram_controller[chan_id]->transaction_queue->transaction_count;
	}
	aux_stat->mem_gather_stat(GATHER_TRAN_QUEUE_VALID_STAT, total_trans);
}

void DRAM::gather_biu_slot_stats() {
	//aux_stat->mem_gather_stat(GATHER_BIU_SLOT_VALID_STAT, biu->active_slots);
}

bool DRAM::schedule_new_transaction(int thread_id, uint64_t address,
		int trans_type, uint64_t request_id) {
	int transaction_index;
	int next_slot_id;
	Transaction *this_t = NULL;
	///uint64_t    address = 0;
	DRAMaddress this_a;
	///int		thread_id;
	/* Then if the command bus(ses) are still open and idle, and something *could* be scheduled,
	 * but isn't, then we can look to create a new transaction by looking in the biu for pending requests
	 * FBDIMM : You check the up_bus
	 * If Refresh enabled you check if you have a refresh waiting ... */

	/// if((next_slot_id = biu->get_next_request_from_biu()) != MEM_STATE_INVALID){
	/* If command bus is idle, see if there is another request in BIU that needs to be serviced.
	 * We start by finding the request we want to service.
	 * Specifically, we want the slot_id of the request
	 * and either move it from VALID to MEM_STATE_SCHEDULED  or from MEM_STATE_SCHEDULED to COMPLETED
	 */

	///thread_id = biu->get_thread_id(next_slot_id);

	///address 			= biu->get_physical_address( next_slot_id);
	this_a.physical_address = address;
	// We need to setup an option in case address is already broken down!
	if (this_a.convert_address(config->physical_address_mapping_policy, config)
			== ADDRESS_MAPPING_FAILURE) {
		/* ignore address mapping failure for now... */
	}
	//	  if(thread_id != -1){
	if (thread_id != -1) {
		int scramble_id = config->thread_set_map[thread_id];
		this_a.chan_id = (this_a.chan_id + scramble_id) % config->channel_count;
		this_a.rank_id = (this_a.rank_id + scramble_id) % config->rank_count;
		this_a.bank_id = (this_a.bank_id + scramble_id) % config->bank_count;
		this_a.row_id = (this_a.row_id + scramble_id) % config->row_count;
	}

	this_a.thread_id = thread_id;
	int new_chan_id = this_a.chan_id;

	if (dram_controller[new_chan_id]->transaction_queue->transaction_count
			>= (config->max_tq_size)) {
		if (config->tq_info.get_transaction_debug())
			fprintf(
					stdout,"[%llu] Transaction queue [%d] full. Cannot start new transaction.\n",
					current_dram_time, new_chan_id);

		return false;
	}

	this_t
			= dram_controller[new_chan_id]->transaction_queue->add_transaction(
					current_dram_time,
					///biu->get_access_type(next_slot_id),
					trans_type,
					thread_id,
					request_id,
					&this_a,
					&(dram_controller[this_a.chan_id]->rank[this_a.rank_id].bank[this_a.bank_id]));

	//fprintf(stdout, "adding new transaction, transaction %d, request_id %d, thread_id %d addr %d, count %d\n", this_t->transaction_id, this_t->request_id, this_t->thread_id, this_t->address.physical_address, dram_controller[new_chan_id]->transaction_queue->transaction_count);


	transaction_index = this_t->tindex;
	/* If the transaction selection policy is MOST/LEAST than you sort it */
	int trans_sel_policy = config->get_transaction_selection_policy();
	if (trans_sel_policy == MOST_PENDING || trans_sel_policy == LEAST_PENDING) {
		int
				rank_id =
						dram_controller[new_chan_id]->transaction_queue->entry[transaction_index].address.rank_id;
		int
				bank_id =
						dram_controller[new_chan_id]->transaction_queue->entry[transaction_index].address.bank_id;
		int
				row_id =
						dram_controller[new_chan_id]->transaction_queue->entry[transaction_index].address.row_id;
		int
				trans_id =
						dram_controller[new_chan_id]->transaction_queue->entry[transaction_index].transaction_id;

		dram_controller[new_chan_id]->pending_queue.add_trans_pending_queue(
				trans_id, rank_id, bank_id, row_id);
	}

	if (this_t->transaction_type == MEMORY_WRITE_COMMAND) {
		if (dram_controller[this_t->address.chan_id]->active_write_trans++ > 4)
			dram_controller[this_t->address.chan_id]->active_write_flag = true;
	}

	if (transaction_index != MEM_STATE_INVALID) {
		///  biu->set_last_transaction_type( biu->get_access_type( next_slot_id));
		///  biu->set_biu_slot_status( next_slot_id, MEM_STATE_SCHEDULED);
		if (config->tq_info.get_transaction_debug()) {
			fprintf(stdout,"Start @ [%llu]: ", current_dram_time);
			Transaction
					trans =
							dram_controller[new_chan_id]->transaction_queue->entry[transaction_index];
			///trans.print_transaction(current_dram_time, biu->get_rid(trans.slot_id));
		}
	} else if (config->tq_info.get_transaction_debug()) {
		fprintf(
				stdout,"Transaction queue full. Cannot start new transaction.\n");
		return false;
	}
	/// }

	return true;
}
