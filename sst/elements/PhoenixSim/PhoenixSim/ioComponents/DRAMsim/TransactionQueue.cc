#include "TransactionQueue.h"



TransactionQueue::TransactionQueue(Global_TQ_info* inf, DRAM_config* cfg, Aux_Stat* st){
	config = cfg;
	tq_info = inf;
	aux_stat = st;
	init_transaction_queue();

}


void TransactionQueue::init_transaction_queue(){
  int i;
  int j;
  /* initialize entries in the transaction queue */



  for(i=0;i<MAX_TRANSACTION_QUEUE_DEPTH;i++){

	  entry[i].status 	= MEM_STATE_INVALID;
	  entry[i].next_c 	= NULL;
	  transaction_count		= 0;

  }



}


int TransactionQueue::get_transaction_debug(){
	return tq_info->get_transaction_debug();
}

/*** This command takes the tran_index i.e. location in the transaction queue
 * as an argument
 * tran_id - Unique Transaction Identifier
 * data_word and data_word_position -> extra parameters added for FBDIMM only
 *  data_word : signifies size of data chunk that drive / cas with drive
 *  dispatches. cacheline_size/DATA_BYTES_PER_READ_BUNDLE
 *  data_word_position : signfies if its the first or the last or middle data
 *  chunks for write data
 *  cmd_id : gives the command id used for statistcs
 * ***/
Command *TransactionQueue::add_command(tick_t now,
	Command *command_queue,
	int command,
	DRAMaddress *this_a,
	uint64_t tran_id,
	int data_word,
	int data_word_position,
	int cmd_id){

  Command *temp_c;
  Command *this_c;

  temp_c = acquire_command();
  temp_c->start_time     	= now;
  temp_c->status     	= IN_QUEUE;
  temp_c->command        	= command;
  temp_c->tran_id			= tran_id;
  temp_c->chan_id      	= this_a->chan_id;
  temp_c->rank_id      	= this_a->rank_id;
  temp_c->bank_id        	= this_a->bank_id;
  temp_c->row_id  	= this_a->row_id;
  temp_c->col_id 		= this_a->col_id;
  temp_c->next_c         	= NULL;
  temp_c->posted_cas		= FALSE; /** NOTE : Used only in FBDIMM **/
  temp_c->refresh	= FALSE;
  temp_c->rq_next_c	= NULL;
  /** Only used for Data, Drive , Cas with drive in FBDIMM **/
  temp_c->data_word = data_word;
  temp_c->data_word_position = data_word_position;

  temp_c->cmd_id =  cmd_id;
  temp_c->link_comm_tran_comp_time = 0;
  temp_c->amb_proc_comp_time = 0;
  temp_c->dimm_comm_tran_comp_time = 0;
  temp_c->dimm_data_tran_comp_time = 0;
  temp_c->dram_proc_comp_time = 0;
  temp_c->amb_down_proc_comp_time = 0;
  temp_c->link_data_tran_comp_time = 0;

  if(command_queue == NULL){
	command_queue = temp_c;
  } else {
	this_c = command_queue;
	while (this_c->next_c != NULL) {
	  this_c = this_c->next_c;
	}
	this_c->next_c = temp_c; /* attach command at end of queue */
  }
  return command_queue;
}


/* release Command into empty command pool */

void TransactionQueue::release_command(Command *freed_command){
  freed_command->next_c = tq_info->free_command_pool;
  tq_info->free_command_pool = freed_command;
}

/* grab empty command thing from queue */

Command *TransactionQueue::acquire_command(){
  Command        *temp_c;

  if(tq_info->free_command_pool == NULL){
	temp_c = new Command();
  } else {
	temp_c = tq_info->free_command_pool;
	tq_info->free_command_pool = tq_info->free_command_pool -> next_c;
  }
  temp_c->next_c = NULL;
  return temp_c;
}

void  TransactionQueue::collapse_cmd_queue(Transaction * this_t,Command *cmd_issued){
  /*
   * We issued a CAS/CASW but has an non-issued RAS/PRE at front - get rid of
   * it
   */
  Command * temp_c;
  // Get rid of precharge
  Command * prev_c = NULL;
  temp_c = this_t->next_c;
  while (temp_c != NULL && temp_c->cmd_id != cmd_issued->cmd_id) {
	if (temp_c->status == IN_QUEUE && temp_c->command != DATA) {
	  if (prev_c == NULL) { // First chappie in the queuer
		  this_t->next_c= temp_c->next_c;
		  // prev_c still NULL
	  } else { // Deleting Middle Item
		prev_c->next_c = temp_c->next_c;
		// prev_c non null
	  }
  	 temp_c->next_c = tq_info->free_command_pool;
	 tq_info->free_command_pool = temp_c;
	  if (prev_c == NULL) { // First chappie in the queue
		temp_c = this_t->next_c;
	  }else {
		temp_c = prev_c->next_c;
	  }
	}else {
	  prev_c = temp_c;
	  temp_c = temp_c->next_c;
	}
  }

}


int TransactionQueue::get_transaction_index(uint64_t transaction_id){
	int i;
	for (i=0;i<transaction_count; i++) {
		if (entry[i].transaction_id == transaction_id)
			return i;
	}
	return MEM_STATE_INVALID;
}

Transaction* TransactionQueue::add_transaction(	tick_t now, int transaction_type, int thread_id, uint64_t request_id, DRAMaddress *this_a, Bank* this_b ){
	Transaction   *this_t = NULL;


	int		transaction_index;




	if(transaction_count >= (config->max_tq_size)){
	  return NULL;
	}
	transaction_index = transaction_count;
	if (transaction_count <= 0) {
	  tq_info->last_transaction_retired_time = now;
	  //fprintf(stdout,"Reset retired time to %llu\n",tq_info->last_transaction_retired_time);
	}

	this_t = &(entry[transaction_index]);	/* in order queue. Grab next entry */

	this_t->status 			= MEM_STATE_VALID;
	this_t->request_id		= request_id;
	this_t->arrival_time 		= now;
	this_t->completion_time 	= 0;
	this_t->transaction_type 	= transaction_type;
	this_t->transaction_id 	= tq_info->transaction_id_ctr++;
	this_t->thread_id		= thread_id;
	this_t->critical_word_ready	= FALSE;
	this_t->critical_word_available	= FALSE;
	this_t->critical_word_ready_time= 0;
	this_t->issued_data = false;
	this_t->issued_command = false;
	this_t->issued_cas = false;
	this_t->issued_ras = false;
	this_t->tindex = transaction_index;


	// Update DRAMaddress
	this_t->address.physical_address= this_a->physical_address;
	this_t->address.chan_id= this_a->chan_id;
	this_t->address.bank_id= this_a->bank_id;
	this_t->address.rank_id= this_a->rank_id;
	this_t->address.row_id= this_a->row_id;
	  /* STATS FOR RANK & CHANNEL */
	  rank_distrib[ this_a->chan_id ][ this_a->rank_id ]++;
	  tot_reqs++;

	  /* */

	  //locality stat disabled.  needs to be fixed, if it actually is useful
	//aux_stat->mem_gather_stat(GATHER_REQUEST_LOCALITY_STAT, (int)this_a->physical_address);		/* send this address to the stat collection */

	if(get_transaction_debug()){
		this_a->print_address();
	}
	/*
	//Ohm--stat for dram_access
	dram_controller[this_a->chan_id].rank[this_a->rank_id].r_p_info.dram_access++;
	dram_controller[this_a->chan_id].rank[this_a->rank_id].r_p_gblinfo.dram_access++;
	*/

	this_t->next_c = transaction2commands(now,this_t->transaction_id,transaction_type,this_a, this_b);
	this_t->status = MEM_STATE_SCHEDULED;
	transaction_count++;


	return this_t;
}

void TransactionQueue::remove_transaction(int tindex, int current_dram_time){
	Transaction   *this_t;
	Command	*this_c;
	int		i;
	int		victim_index;


	if(get_transaction_debug()){
	  if ( entry[tindex].transaction_type != MEMORY_WRITE_COMMAND) {
         fprintf(stdout,"[%llu]Removing transaction %llu at [%d] Time in Queue %llu Memory Latency %llu\n",
             current_dram_time, entry[tindex].transaction_id,tindex, current_dram_time - entry[tindex].arrival_time,entry[tindex].critical_word_ready_time - entry[tindex].arrival_time );
               } else {
                             fprintf(stdout,"[%llu] Removing transaction %llu at [%d] Time in Queue %llu \n",
             current_dram_time,entry[tindex].transaction_id, tindex, current_dram_time - entry[tindex].arrival_time );



	  }
	}
	tq_info->last_transaction_retired_time = current_dram_time;
	this_t =  &(entry[tindex]);
	this_c = this_t->next_c;
	while(this_c->next_c != NULL){
		this_c = this_c->next_c;
	}
	this_c->next_c = tq_info->free_command_pool;
	tq_info->free_command_pool = this_t->next_c;
	this_t ->next_c = NULL;

	for(i = tindex; i < (transaction_count - 1) ; i++){
		entry[i] 			= entry[i+1];
		entry[i].tindex 	= i;
	}
	transaction_count--;
	victim_index = transaction_count;
	entry[victim_index].status 			= MEM_STATE_INVALID;
	entry[victim_index].arrival_time 			= 0;
	entry[victim_index].completion_time 		= 0;
	entry[victim_index].transaction_type 		= MEM_STATE_INVALID;
	entry[victim_index].transaction_id 		= 0;
	entry[victim_index].request_id = -1;
	entry[victim_index].thread_id = -1;
	entry[victim_index].slot_id = MEM_STATE_INVALID;
	entry[victim_index].address.chan_id       = MEM_STATE_INVALID;
    entry[victim_index].address.physical_address      = 0;
	entry[victim_index].critical_word_ready		= FALSE;
	entry[victim_index].critical_word_available		= FALSE;
	entry[victim_index].critical_word_ready_time	= 0;
	entry[victim_index].next_c 			= NULL;
	entry[victim_index].next_ptr 			= NULL;
	entry[victim_index].issued_data = false;
	entry[victim_index].issued_command = false;
	entry[victim_index].issued_cas = false;

}



/* This function takes the translated address, inserts command sequences into a command queue
   This is a little bit hairy, since we need to create command chains here that needs to check
   against the current state of the memory system as well as previously scheduled transactions
   that had not yet executed.
   */

Command *TransactionQueue::transaction2commands(tick_t now, int tran_id,int transaction_type,DRAMaddress *this_a, Bank* this_b){
	//Command *temp_c;
	Command *command_queue;

	int bank_open;
	int bank_conflict;
	int chan_id,bank_id,row_id,rank_id;
	int i;
	int cas_command;
	int cmd_id = 0;

    command_queue = NULL;
    chan_id = this_a->chan_id;
    bank_id = this_a->bank_id;
    row_id = this_a->row_id;
    rank_id = this_a->rank_id;

    bank_open = FALSE;
    bank_conflict = FALSE;
    //bool bank_hit = FALSE;


    /*if(this_b->status == ACTIVE) {
      if(this_b->row_id == row_id){
      bank_open = TRUE;
      } else {
      bank_conflict = TRUE;
      }
      } deprecated in newer version of memtest from dave 07/28/2004*/
    if(this_b->row_id == row_id){
      this_b->cas_count_since_ras++;
      if(this_b->status == ACTIVE) {
        //fprintf(stdout, " Bank opened by previous transaction \n");
        bank_open = TRUE;
        //bank_hit = TRUE;
      }
    } else {
      aux_stat->mem_gather_stat(GATHER_CAS_PER_RAS_STAT, this_b->cas_count_since_ras);
      this_b->cas_count_since_ras = 0;
      if(this_b->status == ACTIVE) {
        bank_conflict = TRUE;
      }
    }
    bank_conflict = TRUE;
    bank_open= TRUE;
    if(transaction_type == MEMORY_WRITE_COMMAND ){
      cas_command = CAS_WRITE;
    } else {
      ///if (config->dram_type == FBD_DDR2 && config->cas_with_drive == TRUE) {
      ///  cas_command = CAS_WITH_DRIVE;
      ///}else {
        cas_command = CAS;
     /// }
    }
    if((config->row_buffer_management_policy == OPEN_PAGE)){
      /* current policy is open page, so we need to check if the row and bank address match */
      /* If they match, we only need a CAS.  If bank hit, and different row, then we need a */
      /* precharge then RAS. */

      /*** FBDIMM : For a CAS WRITE send Data commands
       * 			Number of Data commands = (cacheline / number of bytes
       * 			per outgoing packet
       * ****/
      if (config->dram_type == FBD_DDR2 && cas_command == CAS_WRITE) {
        for (i=0; i < config->data_cmd_count; i++) {
          int dpos = (i == 0)? DATA_FIRST: (i == config->data_cmd_count - 1) ? DATA_LAST : DATA_MIDDLE;
          command_queue = add_command(now,
              command_queue,
              DATA,
              this_a,
              tran_id,
              0,
              dpos, -1);
        }
      }


      if(bank_conflict == TRUE){  			/* or if we run into open bank limitations */
        command_queue = add_command(now,
            command_queue,
            PRECHARGE,
            this_a,
            tran_id,0,0,cmd_id++);

        command_queue = add_command(now,
            command_queue,
            RAS,
            this_a,
            tran_id,0,0,cmd_id++);

      } else if(bank_open == FALSE){ 			/* insert RAS */
        command_queue = add_command(now,
            command_queue,
            RAS,
            this_a,
            tran_id,0,0,cmd_id++);
      } else {
      }
      for(i=0;i<config->packet_count;i++){ /* add CAS. How many do we add? */
        int dpos = 0;
        int dword = 0;
        if (cas_command == CAS_WITH_DRIVE) {
          dpos = i == 0? DATA_FIRST: i == config->data_cmd_count - 1 ? DATA_LAST : DATA_MIDDLE;

          dword = config->cacheline_size/DATA_BYTES_PER_READ_BUNDLE;
        }

        command_queue = add_command(now,
            command_queue,
            cas_command,
            this_a,
            tran_id,
            dword,
            dpos, cmd_id++);
      }
      /*** FBDIMM: For a Drive command currently add one **/
      // If CAS With Drive, no need of DRIVE commands -aj (unless of course you wish to add more drives)
      if (config->dram_type == FBD_DDR2 && cas_command == CAS) {
        command_queue = add_command(now,
            command_queue,
            DRIVE,
            this_a,
            tran_id,
            (config->cacheline_size)/DATA_BYTES_PER_READ_BUNDLE,
            DATA_LAST,
            cmd_id++);
      }

    } else if ((config->row_buffer_management_policy == CLOSE_PAGE)) {

      /*** FBDIMM : For a CAS WRITE send Data commands
       * 			Number of Data commands = (cacheline / number of bytes
       * 			per outgoing packet
       * ****/
      if (config->dram_type == FBD_DDR2 && cas_command == CAS_WRITE) {
        for (i=0; i < config->data_cmd_count; i++) {
          int dpos = i == 0? DATA_FIRST: i == config->data_cmd_count - 1 ? DATA_LAST : DATA_MIDDLE;
          command_queue = add_command(now,
              command_queue,
              DATA,
              this_a,
              tran_id,1,dpos,-1);
        }
      }

      /* We always need RAS, then CAS, then precharge */
      /* this needs to be fixed for the case that two consecutive accesses may hit the same page */
      command_queue = add_command(now,
          command_queue,
          RAS,
          this_a,
          tran_id,0,0,cmd_id++);
      for(i=0;i<config->packet_count;i++){
        int dpos = 0;
        int dword = 0;
        if (cas_command == CAS_WITH_DRIVE) {
          dpos = i == 0? DATA_FIRST: i == config->data_cmd_count - 1 ? DATA_LAST : DATA_MIDDLE;
          dword = config->cacheline_size/DATA_BYTES_PER_READ_BUNDLE;
        }
        if (config->cas_with_prec) {
          if (transaction_type == MEMORY_WRITE_COMMAND) {
            command_queue = add_command(now,
                command_queue,
                CAS_WRITE_AND_PRECHARGE,
                this_a,
                tran_id,dword,dpos,cmd_id++);

          }else { // No support for cas with drive currently
            command_queue = add_command(now,
                command_queue,
                CAS_AND_PRECHARGE,
                this_a,
                tran_id,dword,dpos,cmd_id++);

          }
        }else {
          command_queue = add_command(now,
              command_queue,
              cas_command,
              this_a,
              tran_id,dword,dpos,cmd_id++);
        }
      }
      /*** FBDIMM: For a Drive command currently add one **/
      if (config->dram_type == FBD_DDR2 && cas_command == CAS) {
        command_queue = add_command(now,
            command_queue,
            DRIVE,
            this_a,
            tran_id,
            (config->cacheline_size/DATA_BYTES_PER_READ_BUNDLE),
            DATA_LAST,
            cmd_id++);
      }

      if (!config->cas_with_prec) {
        command_queue = add_command(now,
            command_queue,
            PRECHARGE,
            this_a,
            tran_id,0,0,cmd_id++);
      }
    } else if (config->row_buffer_management_policy == PERFECT_PAGE) {
      /* "Perfect" buffer management policy only need CAS.  */
      /* suppress cross checking issue, just use CAS for this policy */
      for(i=0;i<config->packet_count;i++){
        int dpos = 0;
        int dword = 0;
        if (cas_command == CAS_WITH_DRIVE) {
          dpos = i == 0? DATA_FIRST: i == config->data_cmd_count - 1 ? DATA_LAST : DATA_MIDDLE;
          dword = config->cacheline_size/DATA_BYTES_PER_READ_BUNDLE;
        }
        command_queue = add_command(now,
            command_queue,
            cas_command,
            this_a,
            tran_id,dword,dpos,cmd_id++);
      }
    } else {
      fprintf(stdout,"I am confused. Unknown buffer mangement policy %d\n ",
          config->row_buffer_management_policy);
    }
    return command_queue;
}





void TransactionQueue::print_bundle(tick_t now,  Command *bundle[],int chan_id){
  int i;
  int cmd_cnt = 0;
  fprintf(stdout,">>>>>>>>>>>> BUNDLE NOW[%llu] chan[%d]",now,chan_id);
  for (i=0;i<BUNDLE_SIZE;i++) {
    if (bundle[i] != NULL) {
      if (bundle[i]->command == DATA)
	cmd_cnt += DATA_CMD_SIZE;
      else
	cmd_cnt++;
    }
  }

  fprintf(stdout," size[%d] \n",cmd_cnt);

  for (i=0; i < BUNDLE_SIZE; i++) {
    if (bundle[i] != NULL) {
      bundle[i]->print_command(now);
    }
  }
  fprintf(stdout,"------------------\n");
  fflush(stdout);
}







