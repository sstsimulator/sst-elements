#include "DRAM_config.h"
#include "TransactionQueue.h"

DRAM_config::DRAM_config(){

	dram_power_config = NULL;

	init_dram_system_configuration();
}



void DRAM_config::init_dram_system_configuration(){

	int i;



	set_dram_type(SDRAM);					/* explicitly initialize the variables */
	set_dram_frequency(100);				/* explicitly initialize the variables */
	set_memory_frequency(100);				/* explicitly initialize the variables */
	set_dram_channel_count(1);    				/* single channel of memory */
	set_dram_channel_width(8);  				/* 8 byte wide data bus */
	set_pa_mapping_policy(SDRAM_BASE_MAP);    /* generic mapping policy */
	set_dram_transaction_granularity(64); 			/* 64 bytes long cachelines */
	set_dram_row_buffer_management_policy(OPEN_PAGE);
	set_cpu_memory_frequency_ratio();
	set_transaction_selection_policy(FCFS);

	strict_ordering_flag		= FALSE;
	packet_count          = MAX(1,(cacheline_size / (channel_width * 8 )));

	t_burst		 	= 	 cacheline_size / (channel_width);
	auto_refresh_enabled		= TRUE;
	refresh_policy			= REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK;
	refresh_issue_policy			= REFRESH_OPPORTUNISTIC;
	refresh_time			= 100;
	refresh_cycle_count	= 10000;
	dram_debug_flag			= FALSE;
	addr_debug_flag			= FALSE;
	wave_debug_flag			= FALSE;
	wave_cas_debug_flag		= FALSE;
	issue_debug_flag		= FALSE;
	var_latency_flag 		= FALSE;
	watch_refresh			= FALSE;
	memory2dram_freq_ratio = 1;

	arrival_threshold = 1500;
	/** Just setup the default types for FBD stuff too **/
	data_cmd_count 			= MAX(1,cacheline_size/DATA_BYTES_PER_WRITE_BUNDLE);
	drive_cmd_count 		= 1;
	num_thread_sets = 1;
	thread_sets[0] = 32;
	max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;
	single_rank = true;

	t_cmd = 0;

   	bus_queue_depth = 0;
   	set_independent_threads();



}

/* Dumps the DRAM configuration. */
void DRAM_config::dump_config(FILE *stream)
{
  fprintf(stream, "===================== DRAM config ======================\n");
  print_dram_type( dram_type, stream );
  fprintf(stream, "  dram_frequency: %d MHz\n", dram_type != FBD_DDR2 ? memory_frequency : dram_frequency );
  fprintf(stream, "  memory_frequency: %d MHz\n", memory_frequency);
  fprintf(stream, "  cpu_frequency: %d MHz\n", get_cpu_frequency());
  fprintf(stream, "  dram_clock_granularity: %d\n", dram_clock_granularity);
  fprintf(stream, "  memfreq2cpufreq ration: %f \n", (float)mem2cpu_clock_ratio);
  fprintf(stream, "  memfreq2dramfreq ration: %f \n", memory2dram_freq_ratio);
  fprintf(stream, "  critical_word_first_flag: %d\n", critical_word_first_flag);
  print_pa_mapping_policy( physical_address_mapping_policy, stream);
  print_row_policy( row_buffer_management_policy, stream);
  print_trans_policy( get_transaction_selection_policy(), stream);
  fprintf(stream, "  cacheline_size: %d\n", cacheline_size);
  fprintf(stream, "  chan_count: %d\n", channel_count);
  fprintf(stream, "  rank_count: %d\n", rank_count);
  fprintf(stream, "  bank_count: %d\n", bank_count);
  fprintf(stream, "  row_count: %d\n", row_count);
  fprintf(stream, "  col_count: %d\n", col_count);
  fprintf(stream, "  buffer count: %d\n", up_buffer_cnt);
  fprintf(stream, "  t_rcd: %d\n", t_rcd);
  fprintf(stream, "  t_cac: %d\n", t_cac);
  fprintf(stream, "  t_cas: %d\n", t_cas);
  fprintf(stream, "  t_ras: %d\n", t_ras);
  fprintf(stream, "  t_rp: %d\n", t_rp);
  fprintf(stream, "  t_cwd: %d\n", t_cwd);
  fprintf(stream, "  t_rtr: %d\n", t_rtr);
  fprintf(stream, "  t_burst: %d\n", t_burst);
  fprintf(stream, "  t_rc: %d\n", t_rc);
  fprintf(stream, "  t_rfc: %d\n", t_rfc);
  fprintf(stream, "  t_al: %d\n", t_al);
  fprintf(stream, "  t_wr: %d\n", t_wr);
  fprintf(stream, "  t_rtp: %d\n", t_rtp);
  fprintf(stream, "  t_dqs: %d\n", t_dqs);

  // FBDIMM RELATED
  fprintf(stream, "  t_amb_up: %d\n", t_amb_up);
  fprintf(stream, "  t_amb_down: %d\n", t_amb_down);
  fprintf(stream, "  t_bundle: %d\n", t_bundle);
  fprintf(stream, "  t_bus: %d\n", t_bus);

  fprintf(stream, "  posted_cas: %d\n", posted_cas_flag);
  fprintf(stream, "  row_command_duration: %d\n", row_command_duration);
  fprintf(stream, "  col_command_duration: %d\n", col_command_duration);

  // REFRESH POLICY
  fprintf(stream, "  auto_refresh_enabled: %d\n", auto_refresh_enabled);
  fprintf(stream, "  auto_refresh_policy: %d\n", refresh_policy);
  fprintf(stream, "  refresh_time: %f\n", refresh_time);
  fprintf(stream, "  refresh_cycle_count: %d\n", refresh_cycle_count);

  fprintf(stream, "  strict_ordering_flag: %d\n", strict_ordering_flag);

  // DEBUG
  fprintf(stream, "  dram_debug: %d\n", dram_debug_flag);
  fprintf(stream, "  addr_debug: %d\n", addr_debug_flag);
  fprintf(stream, "  wave_debug: %d\n", wave_debug_flag);
  fprintf(stream, "========================================================\n\n\n");
}

/**
 * All cycles in the config file are in terms of dram cycles.
 * This converts everything in terms of memory controller cycles.
 */
void DRAM_config::convert_config_dram_cycles_to_mem_cycles(){

    // Not sure wher to put this -> but it needs to be done
    t_cac = t_cas - col_command_duration;
    t_ras*=(int)memory2dram_freq_ratio;              /* interval between ACT and PRECHARGE to same bank */
    t_rcd*=(int)memory2dram_freq_ratio;              /* RAS to CAS delay of same bank */
    t_cas*=(int)memory2dram_freq_ratio;              /* delay between start of CAS command and start of data burst */

    t_cac*=(int)memory2dram_freq_ratio;              /* delay between end of CAS command and start of data burst*/
    t_rp*=(int)memory2dram_freq_ratio;               /* interval between PRECHARGE and ACT to same bank */
                            /* t_rc is simply t_ras + t_rp */
    t_rc*=(int)memory2dram_freq_ratio;
    t_rfc*=(int)memory2dram_freq_ratio;
    t_cwd*=(int)memory2dram_freq_ratio;              /* delay between end of CAS Write command and start of data packet */
    t_rtr*=(int)memory2dram_freq_ratio;              /* delay between start of CAS Write command and start of write retirement command*/
    t_burst*=(int)memory2dram_freq_ratio;           /* number of cycles utilized per cacheline burst */

    t_al*=(int)memory2dram_freq_ratio;               /* additive latency = t_rcd - 2 (ddr half cycles) */
    t_rl*=(int)memory2dram_freq_ratio;               /* read latency  = t_al + t_cas */
    t_wr*=(int)memory2dram_freq_ratio;               /* write recovery time latency, time to restore data o cells */
    t_rtp*=(int)memory2dram_freq_ratio;               /* write recovery time latency, time to restore data o cells */

    //t_bus*=(int)memory2dram_freq_ratio;                  /* FBDIMM - bus delay */
    t_amb_up*=(int)memory2dram_freq_ratio;               /* FBDIMM - Amb up delay */
    t_amb_down*=(int)memory2dram_freq_ratio;             /* FBDIMM - Amb down delay */
    //t_bundle*=(int)memory2dram_freq_ratio;               /* FBDIMM number of cycles utilized to transmit a bundle */
    row_command_duration*=(int)memory2dram_freq_ratio; // originally 2 -> goest to 12
    col_command_duration*=(int)memory2dram_freq_ratio;

    t_dqs*=(int)memory2dram_freq_ratio;          /* rank hand off penalty. 0 for SDRAM, 2 for DDR, */

    refresh_cycle_count*=(int)memory2dram_freq_ratio;
	return;
}



/*
 *  set configurations for specific DRAM Systems.
 *
 *  We set the init here so we can use 16 256Mbit parts to get 512MB of total memory
 */

void DRAM_config::set_dram_type(int type){		/* SDRAM, DDR RDRAM etc */



  dram_type = type;

  if (dram_type == SDRAM){				/* PC100 SDRAM -75 chips.  nominal timing assumed */
	channel_width  		=      8;	/* bytes */
	set_dram_frequency(100);
	row_command_duration 	=      1; 	/* cycles */
	col_command_duration 	=      1; 		/* cycles */
	rank_count  		=      4;  		/* for SDRAM and DDR SDRAM, rank_count is DIMM-bank count */
	/* Micron 256 Mbit SDRAM chip data sheet -75 specs used */
	bank_count   		=      4;		/* per rank */
	row_count		= 8*1024;
	col_count		=    512;
	t_ras 	        	=      5; 	/* actually 44ns, so that's 5 cycles */
	t_rp	 		=      2; 	/* actually 20ns, so that's 2 cycles */
	t_rcd  	        	=      2; 	/* actually 20 ns, so that's only 2 cycles */
	t_cas 		        =      2; 	/* this is CAS, use 2 */
	t_al				=  0;
	t_cac 		        = t_cas - col_command_duration;    /* delay in between CAS command and start of read data burst */
	t_cwd 		        =     -1; 	/* SDRAM has no write delay . -1 cycles between end of CASW command and data placement on data bus*/
	t_rtr  	        	=      0; 	/* SDRAM has no write delay, no need for retire packet */
	t_dqs      	=      0;
	t_rc			= t_ras + t_rp;
	t_rfc			= t_rc;
	t_wr  	        	=  2;
	t_rtp  	        	=  1;

	posted_cas_flag	   = 0;
	up_buffer_cnt	= 0;
	t_amb_up		= 0;
	t_amb_down 		= 0;
	t_bundle		= 0;
	t_bus 			= 0;

	tq_delay =  2;	/* 2 DRAM ticks of delay  @ 100 MHz = 20ns */
  max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;

	critical_word_first_flag=   TRUE;
	dram_clock_granularity 	=      1;
  } else if (dram_type == DDRSDRAM){
	channel_width  		=      8;
	set_dram_frequency(200);
	row_command_duration	=  1 * 2;   	/* cycles */
	col_command_duration	=  1 * 2;	/* cycles */
	rank_count	  	=      4;   	/* for SDRAM and DDR SDRAM, rank_count is DIMM-bank count */
	bank_count   		=      4;		/* per rank */
	row_count		= 8*1024;
	col_count		=    512;
	t_ras 	       		=  5 * 2;
	t_rp 	       		=  2 * 2;
	t_al				=  0;
	t_rcd         		=  2 * 2;
	t_cas 	       		=  2 * 2;
	t_cac 		        = t_cas - col_command_duration;    /* delay in between CAS command and start of read data burst */
	t_cwd 	       		=  0 * 2;  	/* not really write delay, but it's the DQS delay */
	t_rtr         		=      0;
	t_dqs      	=      2;
	t_rc			= t_ras + t_rp;
	t_rfc			= t_rc;
	t_wr  	        	=  2; /* FIXME */
	t_rtp  	        	=  2;

	tq_delay =2*2;	/* 4 DRAM ticks of delay @ 200 MHz = 20ns */
  max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;

	critical_word_first_flag=   TRUE;
	dram_clock_granularity	=      2;	/* DDR clock granularity */
  } else if (dram_type == DDR2 || dram_type == DDR3 ){
	channel_width  		=      8;	/* bytes */
	set_dram_frequency(400);
	row_command_duration	=  1 * 2;   	/* cycles */
	col_command_duration	=  1 * 2;	/* cycles */
	rank_count	  	=      4;   	/* for SDRAM and DDR SDRAM, rank_count is DIMM-bank count */
	bank_count   		=      4;	/* per rank , 8 banks in near future.*/
	row_count		= 8*1024;
	col_count		=    512;
	t_ras 	       		=  9 * 2; 	/* 45ns @ 400 mbps is 18 cycles */
	t_rp 	       		=  3 * 2; 	/* 15ns @ 400 mpbs is 6 cycles */
	t_rcd         		=  3 * 2;
	t_cas 	       		=  3 * 2;
	t_al				=  0;
	t_cac 		        = t_cas - col_command_duration;    /* delay in between CAS command and start of read data burst */
	t_cwd 	       		= t_cac;
	t_rtr         		=      0;
	t_dqs      	=  1 * 2;
	t_rc			= t_ras + t_rp;
	t_rfc			= 51; // 127.5 ns
	t_wr  	        	=  2; /* FIXME */
	t_rtp  	        	=  6;

	t_rl = t_al + t_cas;
	t_rrd = 0;
	t_faw = 0;

	tq_delay =2*2;	/* 4 DRAM ticks of delay @ 200 MHz = 20ns */
  max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;

	critical_word_first_flag=   TRUE;
	dram_clock_granularity	=      2;	/* DDR clock granularity */
	cas_with_prec			= true;
  } else if (dram_type == FBD_DDR2) {
	up_channel_width  		=      10;	/* bits */
	down_channel_width  	=      14;	/* bits */
	set_dram_channel_width(8);  				/* 8 byte wide data bus */
	set_memory_frequency(1600);
	set_dram_frequency(400);
	memory2dram_freq_ratio	=	memory_frequency/dram_frequency;
	row_command_duration	=  1 * 2;   	/* cycles */
	col_command_duration	=  1 * 2;	/* cycles */
	rank_count	  	=      4;   	/* for SDRAM and DDR SDRAM, rank_count is DIMM-bank count */
	bank_count   		=      4;	/* per rank , 8 banks in near future.*/
	row_count		= 8*1024;
	col_count		=    512;
	t_ras 	       		=  9 * 2; 	/* 45ns @ 400 mbps is 18 cycles */
	t_rp 	       		=  3 * 2; 	/* 15ns @ 400 mpbs is 6 cycles */
	t_rcd         		=  3 * 2;
	t_cas 	       		=  3 * 2;
	t_cac 		        = t_cas - col_command_duration;    /* delay in between CAS command and start of read data burst */
	t_cwd 	       		= t_cac;
	t_al				=  0;
	t_rtr         		=      0;
	t_dqs      	=  	0;
	t_rc			= t_ras + t_rp;
	t_rfc			= 51; // 127.5 ns
	t_wr  	        	=  12; /* FIXME */
	t_rtp  	        	=  8;

	t_amb_up		=	6;			/*** Arbitrary Value FIXME **/
	t_amb_down		=	6;			/*** Arbitrary Value FIXME **/
	up_buffer_cnt   = 4;			/** Set to as many up buffers as ther are banks in the rank **/
	down_buffer_cnt   = 4;			/** Set to as many up buffers as ther are banks in the rank **/

	tq_delay =2*2;	/* 4 DRAM ticks of delay @ 200 MHz = 20ns */
  max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;

	critical_word_first_flag=   FALSE;
	dram_clock_granularity	=      2;	/* DDR clock granularity */

	data_cmd_count 			= MAX(1,cacheline_size/DATA_BYTES_PER_WRITE_BUNDLE);
	drive_cmd_count 		= 1;
  	//refresh_policy			= REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK;
  	refresh_time			= 100;
  	refresh_cycle_count		= 10000;
	cas_with_prec			= true;
  }
  else {
	fprintf(stdout,"Unknown memory type %d\n",dram_type);
	exit(3);
  }
}

int DRAM_config::get_dram_type(){		/* SDRAM, DDR RDRAM etc */
  return dram_type;
}

void DRAM_config::set_chipset_delay(int delay){
  tq_delay = MAX(1,delay);
}


/*******************************************
 * FBDIMM - This is the frequency of the DRAM
 * For all other configurations the frequency of the
 * DRAM is the freq of the memory controller.
 * *****************************************/
void DRAM_config::set_dram_frequency(int freq){
	if (dram_type != FBD_DDR2) {
		if(freq < MIN_DRAM_FREQUENCY){
		memory_frequency 	= MIN_DRAM_FREQUENCY;		/* MIN DRAM freq */
		} else if (freq > MAX_DRAM_FREQUENCY){
		memory_frequency 	= MAX_DRAM_FREQUENCY;		/* MAX DRAM freq */
		} else {
		memory_frequency 	= freq;
		}
		dram_frequency 	=  memory_frequency;
	}
	else if(freq < MIN_DRAM_FREQUENCY){
		dram_frequency 	= MIN_DRAM_FREQUENCY;		/* MIN DRAM freq */
	} else if (freq > MAX_DRAM_FREQUENCY){
		dram_frequency 	= MAX_DRAM_FREQUENCY;		/* MAX DRAM freq */
	} else {
		dram_frequency 	= freq;
	}
		memory2dram_freq_ratio	=	(float)memory_frequency/dram_frequency;

	if(dram_power_config != NULL)
		dram_power_config->update(freq);
}

void DRAM_config::set_posted_cas(bool flag){
  posted_cas_flag = flag;
}


int DRAM_config::get_dram_frequency(){
  return dram_frequency;
}

int DRAM_config::get_memory_frequency(){
  return memory_frequency;
}

void DRAM_config::set_memory_frequency(int freq) { /* FBD-DIMM only*/
  if(freq < MIN_DRAM_FREQUENCY){
	memory_frequency 	= MIN_DRAM_FREQUENCY;		/* MIN DRAM freq */
  } else if (freq > MAX_DRAM_FREQUENCY){
	memory_frequency 	= MAX_DRAM_FREQUENCY;		/* MAX DRAM freq */
  } else {
	memory_frequency 	= freq;
  }

  set_cpu_memory_frequency_ratio( );
}

int DRAM_config::get_dram_channel_count(){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  return channel_count;
}
void DRAM_config::set_dram_channel_count(int count){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  channel_count = MIN(MAX(1,count),MAX_CHANNEL_COUNT);
}

int DRAM_config::get_dram_rank_count(){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  return rank_count;
}
void DRAM_config::set_dram_rank_count(int count){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  rank_count = MIN(MAX(1,count),MAX_RANK_COUNT);
}

void DRAM_config::set_dram_bank_count(int count){               /* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  bank_count = count;
}
int DRAM_config::get_dram_bank_count(){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  return bank_count;
}

int DRAM_config::get_dram_row_buffer_management_policy(){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  return row_buffer_management_policy;
}
void DRAM_config::set_dram_row_count(int count){                 /* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  row_count = count;
}
void DRAM_config::set_dram_buffer_count(int count){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  up_buffer_cnt= MIN(MAX(1,count),MAX_AMB_BUFFER_COUNT);
  down_buffer_cnt= MIN(MAX(1,count),MAX_AMB_BUFFER_COUNT);
}

int DRAM_config::get_dram_row_count(){
  return row_count;
}

int DRAM_config::get_dram_col_count(){
  return col_count;
}

void DRAM_config::set_dram_col_count(int count){                 /* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  col_count = count;
}

void DRAM_config::set_dram_channel_width(int width){
  channel_width 	= MAX(MIN_CHANNEL_WIDTH,width);			/* smallest width is 2 bytes */
	t_burst		= MAX(1,cacheline_size / channel_width);
}

void DRAM_config::set_dram_transaction_granularity(int size){
 cacheline_size = MAX(MIN_CACHE_LINE_SIZE,size);				/*bytes */
	t_burst		= MAX(1,cacheline_size / channel_width);
	data_cmd_count 			= MAX(1,cacheline_size/DATA_BYTES_PER_WRITE_BUNDLE);
}

void DRAM_config::set_pa_mapping_policy(int policy){
    physical_address_mapping_policy = policy;
}

void DRAM_config::set_dram_row_buffer_management_policy(int policy){
  row_buffer_management_policy = policy;

}

void DRAM_config::set_dram_refresh_policy(int policy){
 refresh_policy = policy;
   refresh_cycle_count = (tick_t)((refresh_time/row_count)*dram_frequency);
	  /* Time between each refresh command */
	if (refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK)
	 refresh_cycle_count =  refresh_cycle_count/(rank_count * bank_count);
	else if (refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK)
	 refresh_cycle_count =  refresh_cycle_count/(rank_count);
	else if (refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK)
	  refresh_cycle_count =  refresh_cycle_count/(bank_count);

}

void DRAM_config::set_dram_refresh_time(int time){
	refresh_time = time;
	 refresh_cycle_count = (refresh_time/row_count)*dram_frequency;
	  /* Time between each refresh command */
	if (refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK)
	  refresh_cycle_count = refresh_cycle_count/(rank_count * bank_count);
	if (refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK)
	  refresh_cycle_count =  refresh_cycle_count/(rank_count );
	else if (refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK)
	  refresh_cycle_count =  refresh_cycle_count/(bank_count);


}
/* oppourtunistic vs highest priority */
void DRAM_config::set_dram_refresh_issue_policy(int policy){
  refresh_issue_policy = policy;
}

void DRAM_config::set_fbd_var_latency_flag(int value) {
    if (value) {
      var_latency_flag = true;
    }else {
      var_latency_flag = false;
    }
}

void DRAM_config::set_dram_debug(int debug_status){
  dram_debug_flag = debug_status;
}

void DRAM_config::set_issue_debug(int debug_status){
  issue_debug_flag = debug_status;
}
void DRAM_config::set_addr_debug(int debug_status){
  addr_debug_flag = debug_status;
}

void DRAM_config::set_wave_debug(int debug_status){
  wave_debug_flag = debug_status;
}

void DRAM_config::set_wave_cas_debug(int debug_status){
  wave_cas_debug_flag = debug_status;
}

void DRAM_config::set_bundle_debug(int debug_status){
  bundle_debug_flag = debug_status;
}

void DRAM_config::set_amb_buffer_debug(int debug_status){
  amb_buffer_debug_flag = debug_status;
}

/** FB-DIMM : set methods **/
void DRAM_config::set_dram_up_channel_width(int width) {
  up_channel_width = width;
  /** FIXME : Add code to set t_burst i.e. transmission time for data
   * packets*/
}

void DRAM_config::set_dram_down_channel_width(int width) {
 down_channel_width = width;
  /** FIXME : Add code to set t_burst i.e. transmission time for data
   * packets*/
}

void DRAM_config::set_t_bundle(int bundle) {
  t_bundle = bundle;
}

void DRAM_config::set_t_bus(int bus) {
  t_bus = bus;
}

/* This has to be enabled late */

void DRAM_config::enable_auto_refresh(int flag, int refresh_time_ms){
  if(flag == TRUE){
	auto_refresh_enabled 	= TRUE;
	refresh_time 		= refresh_time_ms * 1000.0;	/* input is in milliseconds, keep here in microseconds */
	refresh_cycle_count 		= (int) (refresh_time * dram_frequency/row_count);
	if (refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK)
	  refresh_cycle_count =  refresh_cycle_count/(rank_count * bank_count);
	if (refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK)
	  refresh_cycle_count =  refresh_cycle_count/(rank_count );
	else if (refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK)
	  refresh_cycle_count =  refresh_cycle_count/(bank_count);

  } else {
	auto_refresh_enabled 	= FALSE;
  }
}


void DRAM_config::set_independent_threads(){
  int i;
  int j;
  int num_threads = 0;

  for (i=0;i<num_thread_sets;i++) {
	for (j=0;j<thread_sets[i];j++) {
		thread_set_map[num_threads++] = i;
	}
  }
}


 int DRAM_config::addr_debug(){
  return (addr_debug_flag &&
	  (tq_info.transaction_id_ctr >= tq_info.debug_tran_id_threshold));
}

 int DRAM_config::dram_debug(){
  return (dram_debug_flag &&
	  (tq_info.transaction_id_ctr >= tq_info.debug_tran_id_threshold));
}

int DRAM_config::wave_debug(){
  return (wave_debug_flag &&
	  (tq_info.transaction_id_ctr >= tq_info.debug_tran_id_threshold));
}

 int DRAM_config::cas_wave_debug(){
  return (wave_cas_debug_flag);
}

 int DRAM_config::bundle_debug(){
  return (bundle_debug_flag &&
	  (tq_info.transaction_id_ctr >= tq_info.debug_tran_id_threshold));
}

 int DRAM_config::amb_buffer_debug(){
  return (amb_buffer_debug_flag &&
	  (tq_info.transaction_id_ctr >= tq_info.debug_tran_id_threshold));
}

void DRAM_config::set_strict_ordering(int flag){
  strict_ordering_flag = flag;
}

void DRAM_config::set_transaction_debug(int debug_status){
  tq_info.debug_flag = debug_status;
}

void DRAM_config::set_debug_tran_id_threshold(uint64_t dtit){
  tq_info.debug_tran_id_threshold = dtit;
}

void DRAM_config::set_tran_watch(uint64_t tran_id){
  tq_info.tran_watch_flag = TRUE;
  tq_info.tran_watch_id = tran_id;
}

int DRAM_config::get_tran_watch(uint64_t tran_id) {
  return tq_info.tran_watch_flag &&
  tq_info.tran_watch_id == tran_id;
}
void DRAM_config::set_ref_tran_watch(uint64_t tran_id){
  watch_refresh = TRUE;
  ref_tran_id = tran_id;
}
int DRAM_config::get_ref_tran_watch(uint64_t tran_id) {
  return watch_refresh &&
	ref_tran_id == tran_id;
}



void DRAM_config::print_dram_type( int dram_type, FILE *stream ) {
  fprintf(stream, "  dram_type: ");
  switch( dram_type ) {
    case SDRAM: fprintf(stream, "sdram\n");
      break;
    case DDRSDRAM: fprintf(stream, "ddrsdram\n");
      break;
    case DDR2: fprintf(stream, "ddr2\n");
      break;
    case DDR3: fprintf(stream, "ddr3\n");
      break;
    case FBD_DDR2: fprintf(stream, "fbd_ddr2\n");
      break;
    default: fprintf(stream, "unknown\n");
      break;
  }
}

void DRAM_config::print_pa_mapping_policy( int papolicy, FILE *stream ) {
  fprintf(stream, "  physical address mapping_policy: ");
  switch( papolicy ) {
    case BURGER_BASE_MAP: fprintf(stream, "burger_base_map\n");
      break;
    case BURGER_ALT_MAP: fprintf(stream, "burger_alt_map\n");
      break;
    case SDRAM_BASE_MAP: fprintf(stream, "sdram_base_map\n");
      break;
    case SDRAM_HIPERF_MAP: fprintf(stream, "sdram_hiperf_map\n");
      break;
    case INTEL845G_MAP: fprintf(stream, "intel845g_map\n");
      break;
    case SDRAM_CLOSE_PAGE_MAP: fprintf(stream, "sdram_close_page_map\n");
      break;
    default: fprintf(stream, "unknown\n");
      break;
  }
}

void DRAM_config::print_row_policy( int rowpolicy, FILE *stream ) {
  fprintf(stream, "  row_buffer_management_policy: ");
  switch( rowpolicy ) {
    case OPEN_PAGE: fprintf(stream, "open_page\n");
      break;
    case CLOSE_PAGE: fprintf(stream, "close_page\n");
      break;
    case PERFECT_PAGE: fprintf(stream, "perfect_page\n");
    default: fprintf(stream, "unknown\n");
      break;
  }
}

void DRAM_config::print_refresh_policy( int refreshpolicy, FILE *stream ) {
  fprintf(stream, "  row_buffer_management_policy: ");
  switch( refreshpolicy ) {
    case REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK: fprintf(stream, "refresh_one_chan_all_rank_all_bank\n");
      break;
    case REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK: fprintf(stream, "refresh_one_chan_all_rank_one_bank\n");
      break;
    case REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK: fprintf(stream, "refresh_one_chan_one_rank_one_bank\n");
    default: fprintf(stream, "unknown\n");
      break;
  }
}

void DRAM_config::print_trans_policy( int policy, FILE *stream ) {
  fprintf(stream, "  transaction_selection_policy: ");
  switch( policy ) {
    case WANG: fprintf(stream, "wang\n");
      break;
    case LEAST_PENDING: fprintf(stream, "least pending\n");
			   break;
    case MOST_PENDING: fprintf(stream, "most pending\n");
			   break;
    case OBF: fprintf(stream, "open bank first\n");
      break;
    case RIFF: fprintf(stream, "Read Instruction Fetch First\n");
      break;
    case FCFS: fprintf(stream, "First Come first Serve\n");
			   break;
    case GREEDY: fprintf(stream, "greedy\n");
			   break;
    default: fprintf(stream, "unknown\n");
      break;
  }
}

void DRAM_config::set_cpu_frequency( int freq){
  if(freq < MIN_CPU_FREQUENCY){
	cpu_frequency	= MIN_CPU_FREQUENCY;
	fprintf(stdout,"\n\n\n\n\n\n\n\nWARNING: CPU frequency set to minimum allowed frequency [%d] MHz\n\n\n\n\n\n",cpu_frequency);
  } else if (freq > MAX_CPU_FREQUENCY){
	cpu_frequency	= MAX_CPU_FREQUENCY;
	fprintf(stdout,"\n\n\n\n\n\n\n\nWARNING: CPU frequency set to maximum allowed frequency [%d] MHz\n\n\n\n\n\n",cpu_frequency);
  } else {
	cpu_frequency        = freq;
  }

  set_cpu_memory_frequency_ratio( );
}

int DRAM_config::get_cpu_frequency(){
  return cpu_frequency;
}

/***
 * This function sets the raito of the cpu 2 memory controller frequency
 * Note in FBDIMM only is the memory controller freq diff from the dram freq
 * Old function in Daves original code set_cpu_dram_frequency_ratio
 * **/
void DRAM_config::set_cpu_memory_frequency_ratio( ){

  mem2cpu_clock_ratio  = (double) memory_frequency / (double) cpu_frequency;
  cpu2mem_clock_ratio  = (double) cpu_frequency / (double) memory_frequency;
}

void DRAM_config::set_transaction_selection_policy( int policy){
  transaction_selection_policy = policy;
}
int  DRAM_config::get_transaction_selection_policy(){
  return transaction_selection_policy;
}

int DRAM_config::get_num_ifetch() {
    return tq_info.num_ifetch_tran;
}

int DRAM_config::get_num_prefetch() {
    return tq_info.num_prefetch_tran;
}

int DRAM_config::get_num_read() {
  return tq_info.num_read_tran;
}

int DRAM_config::get_num_write() {
  return tq_info.num_write_tran;
}

int DRAM_config::get_biu_queue_depth (){
    return bus_queue_depth;
}

void DRAM_config::set_biu_depth(int depth){
  bus_queue_depth = MAX(1,depth);
}






