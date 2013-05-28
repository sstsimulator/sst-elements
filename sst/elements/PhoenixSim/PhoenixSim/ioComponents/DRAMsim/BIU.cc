
#include "BIU.h"
#include <assert.h>


/*
 * BIU initialization.
 */

BIU::BIU(DRAM_config* cfg){
	dram_system_config = cfg;
	init_biu();


}

void BIU::init_biu(){
  int i;

  current_cpu_time = 0;

  for(i=0;i<MAX_BUS_QUEUE_DEPTH;i++){
	slot[i].status = MEM_STATE_INVALID;
  }
  delay = 1;
  debug_flag = FALSE;
  for(i=0;i<MEMORY_ACCESS_TYPES_COUNT;i++){
	access_count[i] = 0;
  }
  prefetch_biu_hit = 0;
  current_dram_time = 0;
  last_transaction_type = MEM_STATE_INVALID;
  max_write_burst_depth = 8;			/* sweep at most 8 writes at a time */
  write_burst_count = 0;
  biu_trace_fileptr = NULL;
  active_slots = 0;
  critical_word_rdy_slots = 0;
  fixed_latency_flag = 0;

  prefetch_entry_count = 0;
  last_bank_id = 0;
  last_rank_id = 0;
  thread_count = 0;
}


int  BIU::get_biu_depth(){
  return dram_system_config->bus_queue_depth;
}

void BIU::set_biu_delay( int delay_value){
  delay = MAX(0,delay_value);
}

int  BIU::get_biu_delay(){
  return delay;
}

void  BIU::set_dram_chan_count_in_biu( int count){
  //dram_system_config.channel_count = count;
}

void  BIU::set_dram_cacheline_size_in_biu( int size){
  //dram_system_config.cacheline_size = size;
}

void  BIU::set_dram_chan_width_in_biu( int width){
  //dram_system_config.channel_width = width;
}

void  BIU::set_dram_rank_count_in_biu( int count){
  //dram_system_config.rank_count = count;
  last_rank_id = count - 1;
}

void  BIU::set_dram_bank_count_in_biu( int count){
  //dram_system_config.bank_count = count;
  last_bank_id = count - 1;
}

void  BIU::set_dram_row_count_in_biu( int count){
  //dram_system_config.row_count = count;
}

void  BIU::set_dram_col_count_in_biu( int count){
  //dram_system_config.col_count = count;
}

void  BIU::set_dram_address_mapping_in_biu( int policy){
  //dram_system_config.physical_address_mapping_policy = policy;
}



/*
 * If there is a prefetch mechanism, the prefetch request could be in the biu
 * slot already.  If there is an entry in the BIU already, then return the sid.
 * If the incoming request is a prefetch, and its priority is lower than the
 * existing entry, do nothing, but still return sid.  If its priority is higher
 * than the existing entry, update and go on.
 */

int BIU::biu_hit_check(
	int access_type,
	unsigned int baddr,
	int priority,
	int rid,
	tick_t now){

  int i, match_found;
  int bus_queue_depth = this->get_biu_depth();

  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  match_found = FALSE;
  for(i=0;i<bus_queue_depth && (match_found == FALSE);i++){
	if((slot[i].status != MEM_STATE_INVALID) &&
		(slot[i].address.physical_address == baddr) &&
		(slot[i].access_type == MEMORY_PREFETCH)){
	  if((access_type == MEMORY_IFETCH_COMMAND) ||
		  (access_type == MEMORY_READ_COMMAND)){

		slot[i].rid = rid;
		prefetch_biu_hit++;
		slot[i].access_type = access_type;
		slot[i].callback_done = FALSE;
		match_found = TRUE;
	  } else if (access_type == MEMORY_PREFETCH){
		if(priority < slot[i].priority){
		  slot[i].priority = priority;
		}
		match_found = TRUE;
	  }
	}
  }
  return match_found;
}

/*
 *  Low Priority number == High priority.   Priority of 0 means non-speculative.
 *  This function finds a free slot in the BIU.
 */

int BIU::find_free_biu_slot( int priority){
  int i,victim_id = MEM_STATE_INVALID,victim_found = false;
  int bus_queue_depth = this->get_biu_depth();


  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);

  if (active_slots != bus_queue_depth) {

	for(i=0;i<bus_queue_depth;i++){
	  if(slot[i].status == MEM_STATE_INVALID){
		return i;
	  }
	}
  }
  /* Nothing found, now look for speculative prefetch entry to sacrifice */
  victim_found = FALSE;
  for(i=0;i<bus_queue_depth;i++){
	if((slot[i].access_type == MEMORY_PREFETCH) &&
		(slot[i].status == MEM_STATE_VALID)){
	  if(victim_found){
		if(slot[i].priority > slot[victim_id].priority){	/* higher priority number has less priority */
		  victim_id = i;
		}
	  } else {
		victim_found = TRUE;
		victim_id = i;
	  }
	}
  }
  if((victim_found == TRUE) &&
	  ((priority == MEM_STATE_INVALID) || (priority < slot[victim_id].priority))){
	active_slots--;
	return victim_id;	/* kill the speculative prefetch */
  } else {
	return MEM_STATE_INVALID;		/* didn't find anything */
  }
}

/*
 *  Note Put the entry into BIU slot with slot id
 *  Note that the fill_biu_slot definition has references to the driver code.
 *  To interface to the simulator you will no longer need to modify this
 *  function definition
 */

int BIU::fill_biu_slot(

	int thread_id,

	int rid,
	unsigned int baddr,
	int access_type,
	int priority,
	void *mp,
	RELEASE_FN_TYPE callback_fn


	){
 int now_high;
 int now_low;
#ifdef SIM_MASE
 int i;
#endif

	 int slot_id = find_free_biu_slot(priority);

	 if(slot_id == MEM_STATE_INVALID)
			return false;

  if( biu_debug()){
	//#ifdef ENABLE_STDOUT
	fprintf(stdout,"BIU: acquire sid [%2d] rid [%3d] tid[%3d] access_type[", slot_id, rid,thread_id);
	print_access_type(access_type, stdout);
	fprintf(stdout, "] addr [0x%8X] Now[%d]\n", baddr, (int)current_cpu_time);
	
	//#endif
  }
  if(biu_fixed_latency()){	/* fixed latency, no dram */
	slot[slot_id].status = MEM_STATE_COMPLETED;
	slot[slot_id].start_time = current_cpu_time + get_biu_delay();
  } else {	/* normal mode */
	slot[slot_id].status = MEM_STATE_VALID;
	slot[slot_id].start_time = current_cpu_time;
  }
  slot[slot_id].thread_id = thread_id;
  slot[slot_id].address.thread_id = thread_id;
  slot[slot_id].rid = rid;
#ifdef SIM_MASE
  for(i=0;i<bus_queue_depth;i++){   /* MASE BUG. Spits out same address, so we're going to offset it */
	if((i != slot_id) &&
		(slot[slot_id].address.physical_address == slot[i].address.physical_address) &&
		(slot[slot_id].access_type) == (slot[i].access_type)){
	  slot[slot_id].address.physical_address += dram_system_config->cacheline_size;
	}
  }
#endif
  slot[slot_id].address.physical_address = (unsigned int)baddr;
  slot[slot_id].address.convert_address(dram_system_config->physical_address_mapping_policy,
	  dram_system_config);
  if(thread_id != -1){ // Fake Multi-processor workloads
	slot[slot_id].address.bank_id = (slot[slot_id].address.bank_id + thread_id) %
	  dram_system_config->bank_count;
  }
  slot[slot_id].access_type = access_type;
  slot[slot_id].critical_word_ready = FALSE;
  slot[slot_id].callback_done = FALSE;
  slot[slot_id].priority = priority;
  slot[slot_id].mp = mp;
  slot[slot_id].callback_fn = callback_fn;
  // Update the stats
  active_slots++;
  // Update the time for the last access and gather stats
  aux_stat->mem_gather_stat(GATHER_BIU_ACCESS_DIST_STAT,current_cpu_time - last_transaction_time);
  if(biu_trace_fileptr != NULL){
	fprintf(biu_trace_fileptr,"0x%8X ",baddr);
	switch(access_type){
	  case MEMORY_UNKNOWN_COMMAND:
		fprintf(biu_trace_fileptr,"UNKNOWN");
		break;
	  case MEMORY_IFETCH_COMMAND:
		fprintf(biu_trace_fileptr,"IFETCH ");
		break;
	  case MEMORY_WRITE_COMMAND:
		fprintf(biu_trace_fileptr,"WRITE  ");
		break;
	  case MEMORY_READ_COMMAND:
		fprintf(biu_trace_fileptr,"READ   ");
		break;
	  case MEMORY_PREFETCH:
		fprintf(biu_trace_fileptr,"P_FETCH");
		break;
	  default:
		fprintf(biu_trace_fileptr,"UNKNOWN");
		break;
	}
	now_high = (int)(current_cpu_time / 1000000000);
	now_low = (int)(current_cpu_time - (now_high * 1000000000));
	if(now_high != 0){
	  fprintf(biu_trace_fileptr," %d%09d\n",now_high,now_low);
	} else {
	  fprintf(biu_trace_fileptr," %d\n",now_low);
	}
  }

  return slot_id;
}
/*
 * When a BIU slot is to be released, we know that the data has fully returned.
 */

void BIU::release_biu_slot(  int sid){
  if(biu_debug()){
	fprintf(stdout,"BIU: release sid [%2d] rid [%3d] access_type[", sid, slot[sid].rid);
	print_access_type(slot[sid].access_type,stdout);
	fprintf(stdout,"] addr [0x%8X]\n", (unsigned int)slot[sid].address.physical_address);
  }

  slot[sid].status = MEM_STATE_INVALID;
  slot[sid].thread_id = MEM_STATE_INVALID;
  slot[sid].rid = MEM_STATE_INVALID;
  access_count[slot[sid].access_type]++;
  slot[sid].access_type = MEM_STATE_INVALID;
  slot[sid].critical_word_ready = FALSE;
  slot[sid].callback_done = FALSE;
  slot[sid].callback_fn = NULL;
  active_slots--;
}

/* look in the bus queue to see if there are any requests that needs to be serviced
 * with a specific rank, bank and request type, if yes, return TRUE, it not, return FALSE
 */

int BIU::next_RBRR_RAS_in_biu( int rank_id, int bank_id){
  int i,bus_queue_depth = this->get_biu_depth();

  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  for(i=0;i<bus_queue_depth;i++){
	if((slot[i].status == MEM_STATE_VALID) &&
		((slot[i].access_type == MEMORY_READ_COMMAND) ||
		 (slot[i].access_type == MEMORY_IFETCH_COMMAND)) &&
		(slot[i].address.bank_id == bank_id) &&
		(slot[i].address.rank_id == rank_id) &&
		((current_cpu_time - slot[i].start_time) > delay)){
	  return i;
	}
  }
  return MEM_STATE_INVALID;
}


/* look in the bus queue to see if there are any requests that needs to be serviced
 * Part of the logic exists to ensure that the "slot" only becomes active to the
 * DRAM transaction scheduling mechanism after delay has occured.
 */

int BIU::get_next_request_from_biu(){
  int 	i,found,candidate_id = MEM_STATE_INVALID;
  tick_t	candidate_time = 0;
  int	candidate_priority = 0;
  int	bus_queue_depth = this->get_biu_depth();
  int 	priority_scheme;
  int	last_transaction_type = 0;

  priority_scheme = dram_system_config->get_transaction_selection_policy();
  last_transaction_type = last_transaction_type;


  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  found = FALSE;
  if(priority_scheme == HSTP){
	for(i=0;i<bus_queue_depth;i++){
	  if((slot[i].status == MEM_STATE_VALID) &&
		  ((current_cpu_time - slot[i].start_time) > delay)){
		if(found == FALSE){
		  found = TRUE;
		  candidate_priority = slot[i].priority;
		  candidate_time = slot[i].start_time;
		  candidate_id = i;
		} else if(slot[i].priority < candidate_priority){
		  candidate_priority = slot[i].priority;
		  candidate_time = slot[i].start_time;
		  candidate_id = i;
		} else if(slot[i].priority == candidate_priority){
		  if(slot[i].start_time < candidate_time){
			candidate_priority = slot[i].priority;
			candidate_time = slot[i].start_time;
			candidate_id = i;
		  }
		}
	  }
	}
	if(found == TRUE){
	  return candidate_id;
	} else {
	  return MEM_STATE_INVALID;
	}
  }else {
	  for(i=0;i<bus_queue_depth;i++){
		if((slot[i].status == MEM_STATE_VALID) &&
			((current_cpu_time - slot[i].start_time) > delay)){
		  if(found == FALSE){
			found = TRUE;
			candidate_time = slot[i].start_time;
			candidate_id = i;
		  } else if(slot[i].start_time < candidate_time){
			candidate_time = slot[i].start_time;
			candidate_id = i;
		  }
		}
	  }
	  if(found == TRUE){
		return candidate_id;
	  } else {
		return MEM_STATE_INVALID;
	  }
	}
  return MEM_STATE_INVALID;
}

/* look in the bus queue to see if there are any requests that needs to be serviced
 * Part of the logic exists to ensure that the "slot" only becomes active to the
 * DRAM transaction scheduling mechanism after delay has occured.
 * This mechanism ensures that the biu can move transactions based on commands
 * - address translation is done in the biu. Assuming biu and tq are on the
 *   smae structure -
 */

int BIU::get_next_request_from_biu_chan(int chan_id){
  int 	i,found,candidate_id = MEM_STATE_INVALID;
  tick_t	candidate_time = 0;
  int	candidate_priority = 0;
  int	bus_queue_depth = this->get_biu_depth();
  int 	priority_scheme;
  int	last_transaction_type = 0;

  priority_scheme = dram_system_config->get_transaction_selection_policy();
  last_transaction_type = last_transaction_type;


  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  found = FALSE;
  if(priority_scheme == HSTP){
	for(i=0;i<bus_queue_depth;i++){
	  if((slot[i].status == MEM_STATE_VALID) &&
         (slot[i].address.chan_id == chan_id) &&
		  ((current_cpu_time - slot[i].start_time) > delay)){
		if(found == FALSE){
		  found = TRUE;
		  candidate_priority = slot[i].priority;
		  candidate_time = slot[i].start_time;
		  candidate_id = i;
		} else if(slot[i].priority < candidate_priority &&
         slot[i].address.chan_id == chan_id){
		  candidate_priority = slot[i].priority;
		  candidate_time = slot[i].start_time;
		  candidate_id = i;
		} else if(slot[i].priority == candidate_priority &&
         slot[i].address.chan_id == chan_id){
		  if(slot[i].start_time < candidate_time){
			candidate_priority = slot[i].priority;
			candidate_time = slot[i].start_time;
			candidate_id = i;
		  }
		}
	  }
	}
	if(found == TRUE){
	  return candidate_id;
	} else {
	  return MEM_STATE_INVALID;
	}
  }else {
	  for(i=0;i<bus_queue_depth;i++){
		if((slot[i].status == MEM_STATE_VALID) &&
           (slot[i].address.chan_id == chan_id) &&
			((current_cpu_time - slot[i].start_time) > delay)){
		  if(found == FALSE){
			found = TRUE;
			candidate_time = slot[i].start_time;
			candidate_id = i;
		  } else if(slot[i].start_time < candidate_time &&
           (slot[i].address.chan_id == chan_id)){
			candidate_time = slot[i].start_time;
			candidate_id = i;
		  }
		}
	  }
	  if(found == TRUE){
		return candidate_id;
	  } else {
		return MEM_STATE_INVALID;
	  }
	}
  return MEM_STATE_INVALID;
}


int BIU::bus_queue_status_check( int thread_id){
  int i;
  int bus_queue_depth = this->get_biu_depth();


  if(thread_id == MEM_STATE_INVALID){ 	/* for mase code */
	for(i=0;i<bus_queue_depth;i++){		/* Anything in biu, DRAM must service */
	  if((slot[i].status == MEM_STATE_VALID) || (slot[i].status == MEM_STATE_SCHEDULED)){
		return BUSY;
	  }
	}
  } else { /* for cms code */
	for(i=0;i<bus_queue_depth;i++){
	  if((slot[i].thread_id == thread_id) &&
		  ((slot[i].status == MEM_STATE_VALID) || (slot[i].status == MEM_STATE_SCHEDULED))){
		return BUSY;
	  }
	}
	if(num_free_biu_slot() <= 5){      /* hard coded... */
	  return BUSY;            /* even if I have no requests outstanding, if the bus queue is filled up, it needs to be serviced */
	}
  }
  return IDLE;
}

int BIU::dram_update_required( tick_t current_cpu_time){
  tick_t expected_dram_time;


  expected_dram_time = (tick_t) ((double)current_cpu_time * dram_system_config->mem2cpu_clock_ratio);
  /*
   *                 fprintf(stdout,"cpu[%d] dram[%d] expected [%d]\n",
   *                 		(int)current_cpu_time,
   *                 		(int)current_dram_time,
   *                 		(int)expected_dram_time);
   *
   */
  if((expected_dram_time - current_dram_time) > 1){
	return TRUE;
  } else {
	return FALSE;
  }
}

int BIU::find_critical_word_ready_slot( int thread_id){
  int i;
  int latency;
  int bus_queue_depth = this->get_biu_depth();


  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);

  if (active_slots == 0)
	return MEM_STATE_INVALID;
  if (critical_word_rdy_slots== 0)
	return MEM_STATE_INVALID;

  for(i=0;i<bus_queue_depth;i++){
	if((slot[i].thread_id == thread_id) &&
		(slot[i].critical_word_ready == TRUE) &&
		(slot[i].callback_done == FALSE)) {
      critical_word_rdy_slots--; // Keeps track of those whose callback you want to do
	  slot[i].callback_done = TRUE;
	  latency = (int) (current_cpu_time - slot[i].start_time);
	  if((slot[i].access_type == MEMORY_IFETCH_COMMAND) ||
		  (slot[i].access_type == MEMORY_READ_COMMAND)){	/* gather stat for critical reads only */
		aux_stat->mem_gather_stat(GATHER_BUS_STAT, latency);
		if(biu_debug()){
		  fprintf(stdout,"BIU: Critical Word Received sid [%d] rid [%3d] access_type[",
			  i,
			  slot[i].rid);
		  print_access_type(slot[i].access_type,stdout);
		  fprintf(stdout,"] addr [0x%8X] Now[%d] Latency[%4d]\n",
			  (unsigned int)slot[i].address.physical_address,
			  (int)current_cpu_time,
			  (int)(current_cpu_time-slot[i].start_time));
		}
	  }
	  return i;
	}
  }
  return MEM_STATE_INVALID;
}
int BIU::find_completed_slot( int thread_id, tick_t now){
  int i;
  int access_type,latency;
  int bus_queue_depth = this->get_biu_depth();


  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);

  if (active_slots == 0)
	return MEM_STATE_INVALID;
  for(i=0;i<bus_queue_depth;i++){
	
	if((slot[i].status == MEM_STATE_COMPLETED) && (slot[i].thread_id == thread_id)){
	  access_type = get_access_type(i);
	  latency = (int) (current_cpu_time - slot[i].start_time);
	  if((callback_done(i) == FALSE) && ((access_type == MEMORY_IFETCH_COMMAND) || (access_type == MEMORY_READ_COMMAND))){
		aux_stat->mem_gather_stat(GATHER_BUS_STAT, latency);
	  }
	  return i;
	}
  }
  return MEM_STATE_INVALID;
}
/*
 * Determine the Number of free biu slots currently available
 * Used by simulators to probe for MEM_RETRY situations
 */
int BIU::num_free_biu_slot(){
  int i,j;
  int bus_queue_depth = this->get_biu_depth();


  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  for(i=0,j=0;i<bus_queue_depth;i++){
	if(slot[i].status == MEM_STATE_INVALID){
	  j++;
	}
  }
  return j;
}

int BIU::get_rid( int slot_id){
  return slot[slot_id].rid;
}

int BIU::get_access_type( int slot_id){
  return slot[slot_id].access_type;
}

tick_t BIU::get_start_time( int slot_id){
  return slot[slot_id].start_time;
}

unsigned int BIU::get_virtual_address( int slot_id){
  return slot[slot_id].address.virtual_address;
}

unsigned int BIU::get_physical_address( int slot_id){
  return slot[slot_id].address.physical_address;
}

void BIU::set_critical_word_ready( int slot_id){
  critical_word_rdy_slots++;
  slot[slot_id].critical_word_ready = TRUE;
}

int BIU::critical_word_ready( int slot_id){
  return slot[slot_id].critical_word_ready;
}

void BIU::set_biu_slot_status( int slot_id, int status){
  slot[slot_id].status = status;
}

int BIU::get_biu_slot_status( int slot_id){
  return slot[slot_id].status;
}

int BIU::callback_done( int sid){
  return slot[sid].callback_done;
}
/**
 * Needs to go : retained for legacy interfaces
 */
void BIU::biu_invoke_callback_fn( int slot_id) {

#ifdef ALPHA_SIM
  if (!slot[slot_id].callback_fn) {
	if( slot[slot_id].mp ) {
	  cache_free_access_packet( slot[slot_id].mp );
	}
	return;
  }
  slot[slot_id].callback_fn( sim_cycle, slot[slot_id].mp->obj, slot[slot_id].mp->stamp);
  cache_free_access_packet( slot[slot_id].mp );
#endif
#ifdef SIM_MASE
  // Used by Dave's MEM Test file
  // And MASE
   if (!slot[slot_id].callback_fn) return;
 	 slot[slot_id].callback_fn(slot[slot_id].rid, 1);
#endif
#ifdef GEMS

	 slot[slot_id].callback_fn(slot[slot_id].rid);
#endif
}

RELEASE_FN_TYPE BIU::biu_get_callback_fn(int slot_id) {
 return slot[slot_id].callback_fn;
}

void *BIU::biu_get_access_sim_info(int slot_id) {
//  return slot[slot_id].
  return NULL;
}
void BIU::set_biu_fixed_latency( int flag_status){
  fixed_latency_flag = flag_status;
}

int  BIU::biu_fixed_latency(){
  return fixed_latency_flag;
}

void BIU::set_biu_debug( int debug_status){
  debug_flag = debug_status;
}

int BIU::biu_debug(){
  return debug_flag;
}

void BIU::print_access_type(int type,FILE *fileout){
  switch(type){
	case MEMORY_UNKNOWN_COMMAND:
	  fprintf(fileout,"UNKNOWN");
	  break;
	case MEMORY_IFETCH_COMMAND:
	  fprintf(fileout,"IFETCH ");
	  break;
	case MEMORY_WRITE_COMMAND:
	  fprintf(fileout,"WRITE  ");
	  break;
	case MEMORY_READ_COMMAND:
	  fprintf(fileout,"READ   ");
	  break;
	case MEMORY_PREFETCH:
	  fprintf(fileout,"P_FETCH");
	  break;
	default:
	  fprintf(fileout,"UNKNOWN");
	  break;
  }
}

void BIU::print_biu(){
  int i;
  int bus_queue_depth = this->get_biu_depth();

  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  for(i=0;i<bus_queue_depth;i++){
	if(slot[i].status != MEM_STATE_INVALID){
	  fprintf(stdout,"Entry[%2d] Status[%2d] Rid[%8d] Start_time[%8d] Addr[0x%8X] ",
		  i, slot[i].status, slot[i].rid,
		  (int)slot[i].start_time,
		  (unsigned int)slot[i].address.physical_address);
	  print_access_type(slot[i].access_type,stdout);
	  fprintf(stdout,"\n");
	}
  }
}

void BIU::print_biu_access_count(FILE *fileout){
  int i;
  int total_count;
  total_count = 1;
  if (fileout == NULL)
	fileout =stdout;
  for(i=0;i<MEMORY_ACCESS_TYPES_COUNT;i++){
	total_count += access_count[i];
  }
  fprintf(fileout,"TOTAL   Count = [%8d]  percentage = [100.00]\n",total_count);
  for(i=0;i<MEMORY_ACCESS_TYPES_COUNT;i++){
	print_access_type(i,fileout);
	fprintf(fileout," Count = [%8d]  percentage = [%5.2lf]\n",
		access_count[i],
		100.0 * access_count[i]/ (double) total_count);
  }
  fprintf(fileout,"\n");
}

tick_t BIU::get_current_cpu_time(){
  return current_cpu_time;
}

void BIU::set_current_cpu_time( tick_t time){
  current_cpu_time = time;
}



void BIU::set_last_transaction_type( int type){
  last_transaction_type = type;
}
int  BIU::get_last_transaction_type(){
  return last_transaction_type;
}



/* As a thread exits, last thing it should do is to come here and clean of the biu with stuff that belongs to it */
/* CMS specific */

void BIU::scrub_biu( int thread_id){
  int i;
  int bus_queue_depth = this->get_biu_depth();

  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  for(i=0;i<bus_queue_depth;i++){
	if(slot[i].thread_id == thread_id){
	  release_biu_slot(i);
	}
  }
}

int BIU::get_thread_id( int slot_id){
  return slot[slot_id].thread_id;
}

void BIU::set_thread_count( int count){
  thread_count = count;
}

int BIU::get_thread_count(){
  return thread_count;
}
bool BIU::is_biu_busy(){
  bool busy = false;
  if (active_slots > 0) {
	busy = true;
  }
  return busy;
}

void BIU::gather_biu_slot_stats() {
  aux_stat->mem_gather_stat(GATHER_BIU_SLOT_VALID_STAT, active_slots);
}

int BIU::get_biu_queue_depth (){
    return dram_system_config->bus_queue_depth;
}
