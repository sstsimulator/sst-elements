#include "DRAM_Sim.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

DRAM_Sim::DRAM_Sim(BIU* b, DRAM_config *con) {

	config = con;
	biu = b;
	aux_stat = new Aux_Stat(con);
	b->aux_stat = aux_stat;
	dram = new DRAM(b, con, aux_stat);

	callback = NULL;

	transaction_total = 0;
	transaction_retd_total = 0;
	last_retd_total = 0;

	biu_stats = false;
	biu_slot_stats = false;
	biu_access_dist_stats = false;
	bundle_stats = false;
	tran_queue_stats = true;
	bank_hit_stats = false;
	bank_conflict_stats = false;
	cas_per_ras_stats = false;
}
;

void DRAM_Sim::init() {
	sim_start_time = time((time_t *) NULL);

	/* output simulation conditions */
	char *s = ctime(&sim_start_time);
	if (s[strlen(s) - 1] == '\n')
		s[strlen(s) - 1] = '\0';
	//fprintf(stdout, "\nDRAMsim: simulation started @ %s \n", s);

}

void DRAM_Sim::finish() {

	sim_end_time = time((time_t *) NULL);

	/* output simulation conditions */
	/*char *s = ctime(&sim_end_time);
	 if (s[strlen(s) - 1] == '\n')
	 s[strlen(s) - 1] = '\0';
	 fprintf(stdout, "\nDRAMsim: simulation ended @ %s \n", s);
	 int sim_elapsed_time = MAX(sim_end_time - sim_start_time, 1);
	 fprintf(stdout, "Simulation Duration %d\n", sim_elapsed_time);

	 fprintf(stdout, "---------- SIMULATION END -------------\n");
	 fprintf(stdout, "---------- END TIME %llu -------------\n",
	 current_cpu_time);
	 fprintf(stdout, "---------- NUM TRANSACTIONS SIMULATED %u -------\n",
	 transaction_total);
	 fprintf(stdout, "---------- NUM IFETCH TRANSACTIONS %d ----------\n",
	 config->get_num_ifetch());
	 fprintf(stdout, "---------- NUM PREFETCH TRANSACTIONS %d ---------\n",
	 config->get_num_prefetch());
	 fprintf(stdout, "---------- NUM READ TRANSACTIONS %d -------------\n",
	 config->get_num_read());
	 fprintf(stdout, "---------- NUM WRITE TRANSACTIONS %d -------------\n",
	 config->get_num_write());
	 if (biu_stats) {
	 aux_stat->mem_print_stat(GATHER_BUS_STAT, false);
	 }
	 if (bundle_stats) {
	 aux_stat->mem_print_stat(GATHER_BUNDLE_STAT, false);
	 aux_stat->print_extra_bundle_stats(false);
	 }
	 if (tran_queue_stats) {
	 aux_stat->mem_print_stat(GATHER_TRAN_QUEUE_VALID_STAT, false);
	 }
	 if (biu_slot_stats) {
	 aux_stat->mem_print_stat(GATHER_BIU_SLOT_VALID_STAT, false);
	 }
	 if (biu_access_dist_stats) {
	 biu->print_biu_access_count(NULL);
	 aux_stat->mem_print_stat(GATHER_BIU_ACCESS_DIST_STAT, false);
	 }

	 fprintf(stdout, "----- POWER STATS ----\n");
	 dram->power_config->print_global_power_stats(stdout,
	 dram->current_dram_time);
	 dram->print_transaction_queue();
	 //fcloseall();*/

}

/**************************************************************************************/
/*********************************** DRAM_Sim with trace file********************************/

DRAM_Sim_TRACE::DRAM_Sim_TRACE(BIU* b, DRAM_config *con, int num, char** files) :
	DRAM_Sim(b, con) {
	type = TRACE;
	num_trace = num;
	trace_filein = files;

	if (num_trace != 0) {
		int i;

		for (i = 0; i < num_trace; i++) {
			assert(trace_filein[i] != NULL);
			if ((trace_fileptr[i] = fopen(trace_filein[i], "r")) == NULL) {
				fprintf(stdout, " Error opening trace input %s\n",
						trace_filein[i]);
				exit(0);
			}
		}
		current_bus_event = (BusEvent*) calloc(num_trace, sizeof(BusEvent));
		for (i = 0; i < num_trace; i++) {
			current_bus_event[i].already_run = TRUE;
		}
		if (current_bus_event == NULL) {
			fprintf(stdout, " Error no memory available \n");
			exit(0);
		}
	}
}

DRAM_Sim_TRACE::~DRAM_Sim_TRACE() {
	free(current_bus_event);
}

void DRAM_Sim_TRACE::start() {

	int fake_rid;

	int slot_id;
	int thread_id;
	int get_next_event_flag = FALSE;
	fprintf(stdout, "here\n");
	BusEvent *next_e = get_next_bus_event(num_trace);
	while (current_cpu_time < max_cpu_time && next_e != NULL
			&& next_e->attributes != MEM_STATE_INVALID) {
		if (current_cpu_time > next_e->timestamp) {

			if (add_new_transaction(next_e->trace_id, -1,
					(unsigned int) next_e->address, next_e->attributes,
					MEM_STATE_INVALID)) {
				transaction_total++;
				get_next_event_flag = TRUE;
				next_e->already_run = TRUE;
			}

		}
		thread_id = MEM_STATE_INVALID;
		//fprintf(stdout, "here1\n");
		update();
		//fprintf(stdout, "here2\n");

		if (get_next_event_flag == TRUE) {
			next_e = get_next_bus_event(num_trace);
			get_next_event_flag = FALSE;
		}
		if (current_cpu_time % 100000000000ll == 0) {
			fprintf(
					stdout,
					"---------- TRANS SIM(%d) RETD(%d) TIME %llu -------------\n",
					transaction_total, transaction_retd_total, current_cpu_time);
#if 0
			if (last_retd_total == transaction_retd_total) {
				fprintf(stdout,"-- Error:DEADLOCK DETECTED -- \n");
				dram->print_transaction_queue();
				exit(0);
			}
#endif
			last_retd_total = transaction_retd_total;
		}

	}

}

BusEvent *DRAM_Sim_TRACE::get_next_bus_event(const int num_trace) {
	char input[1024];
	int control;
	double timestamp;
	BusEvent* this_e;

	int i;
	assert(current_bus_event != NULL);
	for (i = 0; i < num_trace; i++) {
		if (current_bus_event[i].already_run == TRUE) {
			if ((fscanf(trace_fileptr[i], "%s", input) != EOF)) {
				sscanf(input, "%X", &(current_bus_event[i].address)); /* found starting Hex address */
				if (fscanf(trace_fileptr[i], "%s", input) == EOF) {
					fprintf(stdout,
					"Unexpected EOF, Please fix input trace file \n");
					exit(2);
				}
				if ((control = trace_file_io_token(input)) == UNKNOWN) {
					fprintf(stdout, "Unknown Token Found [%s]\n", input);
				}
				current_bus_event[i].attributes = control;

				if (fscanf(trace_fileptr[i], "%s", input) == EOF) {
					fprintf(stdout,
					"Unexpected EOF, Please fix input trace file \n");
				}
				sscanf(input, "%lf", &timestamp);
				current_bus_event[i].timestamp = (tick_t) timestamp;
				current_bus_event[i].already_run = false;
				current_bus_event[i].trace_id = i;
			} else {
				current_bus_event[i].attributes = MEM_STATE_INVALID;
			}
		}
	}
	this_e = NULL;
	for (i = 0; this_e == NULL && i < num_trace; i++) {
		if (current_bus_event[i].already_run == false
				&& current_bus_event[i].attributes != MEM_STATE_INVALID) {
			this_e = &current_bus_event[i];
		}
	}
	for (i = 0; i < num_trace; i++) {
		if (this_e->timestamp > current_bus_event[i].timestamp
				&& current_bus_event[i].attributes != MEM_STATE_INVALID) {
			this_e = &current_bus_event[i];
		}
	}
	return this_e;
}

int DRAM_Sim_TRACE::trace_file_io_token(char *input) {
	size_t length;
	length = strlen(input);
	if (strncmp(input, "FETCH", length) == 0) {
		return MEMORY_IFETCH_COMMAND;
	} else if (strncmp(input, "IFETCH", length) == 0) {
		return MEMORY_IFETCH_COMMAND;
	} else if (strncmp(input, "IREAD", length) == 0) {
		return MEMORY_IFETCH_COMMAND;
	} else if (strncmp(input, "P_FETCH", length) == 0) {
		return MEMORY_IFETCH_COMMAND;
	} else if (strncmp(input, "P_LOCK_RD", length) == 0) {
		return MEMORY_READ_COMMAND;
	} else if (strncmp(input, "P_LOCK_WR", length) == 0) {
		return MEMORY_WRITE_COMMAND;
	} else if (strncmp(input, "LOCK_RD", length) == 0) {
		return MEMORY_READ_COMMAND;
	} else if (strncmp(input, "LOCK_WR", length) == 0) {
		return MEMORY_WRITE_COMMAND;
	} else if (strncmp(input, "MEM_RD", length) == 0) {
		return MEMORY_READ_COMMAND;
	} else if (strncmp(input, "WRITE", length) == 0) {
		return MEMORY_WRITE_COMMAND;
	} else if (strncmp(input, "DWRITE", length) == 0) {
		return MEMORY_WRITE_COMMAND;
	} else if (strncmp(input, "MEM_WR", length) == 0) {
		return MEMORY_WRITE_COMMAND;
	} else if (strncmp(input, "READ", length) == 0) {
		return MEMORY_READ_COMMAND;
	} else if (strncmp(input, "DREAD", length) == 0) {
		return MEMORY_READ_COMMAND;
	} else if (strncmp(input, "P_MEM_RD", length) == 0) {
		return MEMORY_READ_COMMAND;
	} else if (strncmp(input, "P_MEM_WR", length) == 0) {
		return MEMORY_WRITE_COMMAND;
	} else if (strncmp(input, "P_I/O_RD", length) == 0) {
		return MEMORY_READ_COMMAND;
	} else if (strncmp(input, "P_I/O_WR", length) == 0) {
		return MEMORY_WRITE_COMMAND;
	} else if (strncmp(input, "IO_RD", length) == 0) {
		return MEMORY_READ_COMMAND;
	} else if (strncmp(input, "I/O_RD", length) == 0) {
		return MEMORY_READ_COMMAND;
	} else if (strncmp(input, "IO_WR", length) == 0) {
		return MEMORY_WRITE_COMMAND;
	} else if (strncmp(input, "I/O_WR", length) == 0) {
		return MEMORY_WRITE_COMMAND;
	} else if (strncmp(input, "P_INT_ACK", length) == 0) {
		return MEMORY_UNKNOWN_COMMAND;
	} else if (strncmp(input, "INT_ACK", length) == 0) {
		return MEMORY_UNKNOWN_COMMAND;
	} else if (strncmp(input, "BOFF", length) == 0) {
		return MEMORY_UNKNOWN_COMMAND;
	} else {
		printf("Unknown %s\n", input);
		return UNKNOWN;
	}
}

/**************************************************************************************/
/*********************************** DRAM_Sim with random traffic ********************************/

DRAM_Sim_RAND::DRAM_Sim_RAND(BIU* b, DRAM_config *con, int type,
		float access_dist[4]) :
	DRAM_Sim(b, con) {
	arrival_distribution_model = type;
	average_interarrival_cycle_count = 10.0; //this is what it is in the original code
	access_distribution = access_dist;

	if (arrival_distribution_model == UNIFORM_DISTRIBUTION) {
		arrival_thresh_hold = 1.0 - (1.0
				/ (double) average_interarrival_cycle_count);
	} else if (arrival_distribution_model == GAUSSIAN_DISTRIBUTION) {
		arrival_thresh_hold = 1.0 - (1.0 / box_muller(
				(double) average_interarrival_cycle_count, 10));
	} else if (arrival_distribution_model == POISSON_DISTRIBUTION) {
		arrival_thresh_hold = 1.0 - (1.0 / poisson_rng(
				(double) average_interarrival_cycle_count));
	}
}

void DRAM_Sim_RAND::start() {
	int slot_id;
	int fake_access_type;
	int fake_rid;
	int fake_v_address; /* virtual address */
	int fake_p_address; /* physical address */
	int thread_id;

	while (current_cpu_time < max_cpu_time) {
		/* check and see if we need to create a new memory reference request */
		if ((double(rand()) / RAND_MAX) > arrival_thresh_hold) { /* interarrival probability function */
			//gaussian distribution function

			if (arrival_distribution_model == GAUSSIAN_DISTRIBUTION) {
				arrival_thresh_hold = 1.0 - (1.0 / box_muller(
						(double) average_interarrival_cycle_count, 10));
			}
			//poisson distribution function
			else if (arrival_distribution_model == POISSON_DISTRIBUTION) {
				arrival_thresh_hold = 1.0 - (1.0 / poisson_rng(
						(double) average_interarrival_cycle_count));
			}

			/* Inject new memory reference */
			fake_access_type = get_mem_access_type(access_distribution);
			if (fake_access_type == MEMORY_READ_COMMAND) {
				fake_rid = 1 + (int) ((double(rand()) / RAND_MAX) * 100000);
			} else {
				fake_rid = 0;
			}
			fake_v_address = (int) ((double(rand()) / RAND_MAX)
					* (unsigned int) (-1));

			fake_p_address = fake_v_address;

			slot_id = biu->find_free_biu_slot(MEM_STATE_INVALID); /* make sure this has highest priority */
			if (slot_id == MEM_STATE_INVALID) {
				/* can't get a free slot, retry later */
			} else {
				/* place request into BIU */
				if (add_new_transaction(0, fake_rid,
						(unsigned int) fake_p_address, fake_access_type,
						MEM_STATE_INVALID))
					transaction_total++; //transaction was successfully added
			}
		}
		update();

		if (current_cpu_time % 1000000 == 0) {
			fprintf(
					stdout,
					"---------- TIME %llu TRANS SIM(%d) RETD(%d)-------------\n",
					current_cpu_time, transaction_total, transaction_retd_total);
#if 1
			if (last_retd_total == transaction_retd_total) {
				fprintf(stdout, "-- DEADLOCK DETECTED -- \n");
				dram->print_transaction_queue();
				exit(0);
			}
#endif
			last_retd_total = transaction_retd_total;
		}

	}

}

/** gaussian number generator ganked from the internet ->
 * http://www.taygeta.com/random/gaussian.html
 */

double DRAM_Sim_RAND::box_muller(double m, double s) {
	double x1, x2, w, y1;
	static double y2;
	static int use_last = 0;

	if (use_last) /* use value from previous call */
	{
		y1 = y2;
		use_last = 0;
	} else {
		do {
			x1 = 2.0 * (double(rand()) / RAND_MAX) - 1.0;
			x2 = 2.0 * (double(rand()) / RAND_MAX) - 1.0;
			w = x1 * x1 + x2 * x2;
		} while (w >= 1.0);

		w = sqrt((-2.0 * log(w)) / w);
		y1 = x1 * w;
		y2 = x2 * w;
		use_last = 1;
	}
	return floor(m + y1 * s);
}

double DRAM_Sim_RAND::gammaln(double xx) {
	double x, y, tmp, ser;
	static double cof[6] = { 76.18009172947146, -86.50532032941677,
			24.01409824083091, -1.231739572450155, 0.1208650973866179e-2,
			-0.5395239384953e-5 };
	int j;

	y = x = xx;
	tmp = x + 5.5;
	tmp -= (x + 0.5) * log(tmp);
	ser = 1.000000000190015;
	for (j = 0; j <= 5; j++)
		ser += cof[j] / ++y;
	return -tmp + log(2.5066282746310005 * ser / x);

}
/**
 * from the book "Numerical Recipes in C: The Art of Scientific Computing"
 **/

double DRAM_Sim_RAND::poisson_rng(double xm) {
	static double sq, alxm, g, oldm = (-1.0);
	double em, t, y;

	if (xm < 12.0) {
		if (xm != oldm) {
			oldm = xm;
			g = exp(-xm);
		}
		em = -1;
		t = 1.0;
		do {
			++em;
			t *= (double(rand()) / RAND_MAX);
		} while (t > g);
	} else {
		if (xm != oldm) {
			oldm = xm;
			sq = sqrt(2.0 * xm);
			alxm = log(xm);
			g = xm * alxm - gammaln(xm + 1.0);
		}
		do {
			do {
				y = tan(PI * (double(rand()) / RAND_MAX));
				em = sq * y + xm;
			} while (em < 0.0);
			em = floor(em);
			t = 0.9 * (1.0 + y * y) * exp(em * alxm - gammaln(em + 1.0) - g);
		} while ((double(rand()) / RAND_MAX) > t);
	}
	return em;
}

int DRAM_Sim_RAND::get_mem_access_type(float *access_distribution) {
	double random_value = (double(rand()) / RAND_MAX);
	int i = 0;
	while (i < MEMORY_ACCESS_TYPES_COUNT - 3) {
		if (random_value < access_distribution[i]) {
			return i + 1;
		}
		i++;
	}
	return MEMORY_ACCESS_TYPES_COUNT - 2;
}

/**************************************************************************************/
/*********************************** DRAM_Sim for MONET ********************************/

DRAM_Sim_MONET::DRAM_Sim_MONET(BIU* biu, DRAM_config* cfg) :
	DRAM_Sim(biu, cfg) { //BIU not going to be used here

}

void DRAM_Sim_MONET::start() {

}

/******************************************************************************************/
/************************************ static initialization stuff***********************/

DRAM_Sim *DRAM_Sim::read_config(double penis, int argc, char *argv[],
		int nothing) {

	int argc_index;
	size_t length;

	int max_inst;
	int cpu_frequency;
	int dram_type; /* dram type {sdram|ddrsdram|drdram|ddr2} */
	int dram_frequency;
	int memory_frequency;
	int channel_count; /* total number of logical channels */
	int channel_width; /* logical channel width in bytes */
	int strict_ordering_flag;
	int refresh_interval; /* in milliseconds */

	int chipset_delay; /* latency through chipset, in DRAM ticks */
	int bus_queue_depth;
	int biu_delay; /* latency through BIU, in CPU ticks */
	int address_mapping_policy; /* {burger_map|burger_alt_map|sdram_base_map|intel845g_map} */
	int row_buffer_management_policy; /* {open_page|close_page|perfect_page|auto_page} */
	int refresh_issue_policy;
	int biu_debug_flag;
	int transaction_debug_flag;
	int dram_debug_flag;

	int all_debug_flag; /* if this is set, all dram related debug flags are turned on */
	int wave_debug_flag; /* cheesy Text Based waveforms  */
	int cas_wave_debug_flag; /* cheesy Text Based waveforms  */
	int bundle_debug_flag; /* Watch Bundles that are issued  */
	int amb_buffer_debug_flag; /* watch amb buffers */
	unsigned int debug_tran_id_threshold; /* tran id at which to start debug output */
	unsigned int debug_tran_id; /* tran id  which to print out why commands are not being output*/
	unsigned int debug_ref_tran_id; /* tran id  which to print out why commands are not being output*/
	int get_next_event_flag = FALSE;

	char *power_stats_fileout = NULL;
	char *common_stats_fileout = NULL;
	char *trace_filein[MAX_TRACES];

	bool biu_stats = false;
	bool biu_slot_stats = false;
	bool biu_access_dist_stats = false;
	bool bundle_stats = false;
	bool tran_queue_stats = true;
	bool bank_hit_stats = false;
	bool bank_conflict_stats = false;
	bool cas_per_ras_stats = false;

	double average_interarrival_cycle_count; /* every X cycles, a transaction arrives */
	double arrival_thresh_hold; /* if drand48() exceed this thresh_hold. */
	int arrival_distribution_model;
	int slot_id;
	int fake_access_type;
	int fake_rid;
	int fake_v_address; /* virtual address */
	int fake_p_address; /* physical address */
	int thread_id;
	char spd_filein[MAX_FILENAME_LENGTH];
	char power_filein[MAX_FILENAME_LENGTH] = "\0";
	FILE *spd_fileptr;
	FILE *power_fileptr;
	float access_distribution[4]; /* used to set distribution precentages of access types */
	int num_trace = 0;
	int transaction_total = 0;
	int transaction_retd_total = 0;
	int last_retd_total = 0;
	char *trace_files[MAX_TRACES];
	int i;
	/* set defaults */

	max_inst = 1000000; /* actually "cycle count" in this tester */
	cpu_frequency = 1000;
	dram_type = SDRAM;
	dram_frequency = 100;
	memory_frequency = 100;
	channel_count = 1;
	channel_width = 8; /* bytes */
	refresh_interval = -1; //Ohm: change this from -1 to 0
	chipset_delay = 10; /* DRAM ticks */
	bus_queue_depth = 32;
	row_buffer_management_policy = MEM_STATE_INVALID;
	chipset_delay = 10; /* DRAM ticks */
	biu_delay = 10; /* CPU ticks */
	address_mapping_policy = SDRAM_BASE_MAP;
	refresh_issue_policy = REFRESH_HIGHEST_PRIORITY;
	strict_ordering_flag = FALSE;
	biu_debug_flag = FALSE;
	transaction_debug_flag = FALSE;
	dram_debug_flag = FALSE;

	all_debug_flag = FALSE;
	wave_debug_flag = FALSE;
	cas_wave_debug_flag = FALSE;
	bundle_debug_flag = FALSE;
	amb_buffer_debug_flag = FALSE;
	average_interarrival_cycle_count = 10.0; /* every 10 cycles, inject a new memory request */
	arrival_distribution_model = UNIFORM_DISTRIBUTION;
	debug_tran_id_threshold = 0;
	debug_tran_id = 0;
	debug_ref_tran_id = 0;
	spd_filein[0] = '\0';
	argc_index = 1;

	// in the below scheme, all access types have equal probability
	access_distribution[0] = 0.20;
	access_distribution[1] = 0.40;
	access_distribution[2] = 0.60;
	access_distribution[3] = 0.80;

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	/* Now init variables */

	DRAM_config* config = new DRAM_config();
	BIU* biu = new BIU(config);
	DRAM_Sim* sim = NULL;

	/* After handling all that, now override default numbers */

	dram_frequency = MEM_STATE_INVALID;
	//channel_count = MEM_STATE_INVALID;
	cpu_frequency = MEM_STATE_INVALID;
	dram_type = MEM_STATE_INVALID;
	channel_width = MEM_STATE_INVALID;

	//fprintf(stdout, "DRAMsim: num args: %d\n", argc);

	if ((argc < 2) || (strncmp(argv[1], "-help", 5) == 0)) {
		fprintf(stdout, "Usage: %s -options optionswitch\n", argv[0]);
		fprintf(stdout, "-trace_file FILENAME\n");
		fprintf(stdout, "-cpu:frequency <frequency of processor>\n");
		fprintf(stdout, "-dram:frequency <frequency of dram>\n");
		fprintf(stdout, "-max:inst <Maximum number of cpu cycles to tick>\n");
		fprintf(
				stdout,
				"-dram:type <DRAM type> allowed options sdram,ddrsdram,ddr,ddr2,drdram,fbdimm\n");
		fprintf(stdout, "-dram:channel_count <number of channels>\n");
		fprintf(stdout, "-dram:channel_width <Data bus width in bytes>\n");
		fprintf(stdout,
		"-dram:refresh <Time to refresh entire dram in ms: default 64 ms>\n");
		fprintf(
				stdout,
				"-dram:refresh_issue_policy <Refresh schedulign priority - options are priority,opportunistic>\n");
		fprintf(stdout, "-dram:refresh_policy <Refresh policy >\n");
		fprintf(stdout,
		"-dram:chipset_delay <Time to get transaction in and out of biu>\n");
		fprintf(stdout,
		"-dram:spd_input <File that lists configuration timing information>\n");
		fprintf(stdout,
		"-dram:power_input <File that lists configuration power information>\n");
		fprintf(stdout,
		"-dram:row_buffer_management_policy <Page policy - open_page,close_page>\n");
		fprintf(
				stdout,
				"-biu:trans_selection <Scheduling policy - fcfs,wang,least_pending,most_pending,greedy,obf(open bank first),riff(read first)>\n");
		fprintf(
				stdout,
				"-dram:address_mapping_policy <Address Mapping policy - burger_base_map,burger_alt_map,sdram_base_map,sdram_close_page_map,intel845g_map>\n");
		fprintf(stdout,
		"-dram:strict_ordering Enable in-order processing of memory requests\n");
		fprintf(stdout,
		"-stat:biu To record distribution of latency of read requests\n");
		fprintf(stdout,
		"-stat:biu_slot To record distribution of memory request types\n");
		fprintf(
				stdout,
				"-stat:biu-access-dist Records the time interval between arrival of two consecutive memory requests\n");
		fprintf(stdout,
		"-stat:bundle Records the composition of a bundle (FBDIMM packet) \n");
		fprintf(stdout,
		"-stat:tran_queue Records the occupancy of the transaction queue\n");
		fprintf(stdout,
		"-stat:dram:bank_hit Open page keeps track of the bank hit statistics\n");
		fprintf(
				stdout,
				"-stat:dram:bank_conflict Open page keeps track of the bank conflict statistics\n");
		fprintf(
				stdout,
				"-stat:power <File that records periodic power consumption patterns on a per rank basis>\n");
		fprintf(
				stdout,
				"-stat:all <File into which all statistics barring periodic power is recorded>\n");

		fprintf(stdout,
		"-debug:wave Displays text-based view of issued commands life-cycle\n");
		fprintf(
				stdout,
				"-debug:cas Displays text-based view of issued column access read and write commands life-cycle\n");
		fprintf(
				stdout,
				"-debug:transaction Displays text-based view of a transactions issued/retired\n");
		fprintf(
				stdout,
				"-debug:biu Displays text-based view of a memory requests issue/retirement in the biu\n");
		fprintf(
				stdout,
				"-debug:bundle Displays text-based view of the FB-DIMM packets that are issued\n");
		fprintf(
				stdout,
				"-debug:amb Displays text-based view of the FB-DIMM AMB occupancy and release\n");
		fprintf(stdout,
		"-debug:threshold <Transaction id after which debug info is displayed>\n");
		fprintf(stdout, "-watch:trans <Transaction id to watch >\n");
		fprintf(stdout, "-watch:ref_trans <Refresh Transaction id to watch >\n");

		fprintf(stdout, "-help To view this message\n");
		exit(0);
	}

	while (argc_index < argc) {
		length = strlen(argv[argc_index]);
		if (strncmp(argv[argc_index], "-sim:MONET", length) == 0) {
			sim = new DRAM_Sim_MONET(biu, config);
			//			fprintf(stdout, "DRAMsim: creating DRAM_Sim object\n");
			argc_index++;
		} else if (strncmp(argv[argc_index], "-sim:TRACE", length) == 0) {
			sim = new DRAM_Sim_TRACE(biu, config, num_trace, trace_files);
			//fprintf(stdout, "DRAMsim: creating DRAM_Sim object\n");
			argc_index++;
		} else if (strncmp(argv[argc_index], "-sim:RAND", length) == 0) {

		} else if (strncmp(argv[argc_index], "-max:inst", length) == 0) {
			sscanf(argv[argc_index + 1], "%d", &max_inst);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-cpu:frequency", length) == 0) {
			sscanf(argv[argc_index + 1], "%d", &cpu_frequency);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:type", length) == 0) {
			length = strlen(argv[argc_index + 1]);
			if (strncmp(argv[argc_index + 1], "sdram", length) == 0) {
				dram_type = SDRAM;
			} else if (strncmp(argv[argc_index + 1], "ddrsdram", length) == 0) {
				dram_type = DDRSDRAM;
			} else if (strncmp(argv[argc_index + 1], "ddr2", length) == 0) {
				dram_type = DDR2;
			} else {
				fprintf(stdout, "Unknown RAM type [%s]\n", argv[argc_index + 1]);
				exit(0);
			}
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:frequency", length) == 0) {
			sscanf(argv[argc_index + 1], "%d", &dram_frequency);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:channel_count", length)
				== 0) {
			sscanf(argv[argc_index + 1], "%d", &channel_count);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:channel_width", length)
				== 0) {
			sscanf(argv[argc_index + 1], "%d", &channel_width);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:refresh", length) == 0) {
			sscanf(argv[argc_index + 1], "%d", &refresh_interval);
			argc_index += 2;

		} else if (strncmp(argv[argc_index], "-dram:chipset_delay", length)
				== 0) {
			sscanf(argv[argc_index + 1], "%d", &channel_width);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:spd_input", length) == 0) {
			sscanf(argv[argc_index + 1], "%s", &spd_filein[0]);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:power_input", length) == 0) {
			sscanf(argv[argc_index + 1], "%s", power_filein);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-trace_file", length) == 0) {
			int j = ++argc_index;

			while ((j < argc) && strncmp(argv[j], "-", 1) != 0) {
				trace_files[num_trace] = (char *) malloc(sizeof(char)
						* MAX_FILENAME_LENGTH);
				sscanf(argv[j], "%s", trace_files[num_trace]);
				num_trace++;
				j++;
			}
			//sim = new DRAM_Sim_TRACE(biu, config, num_trace, trace_files);
			argc_index += num_trace;
		} else if (strncmp(argv[argc_index],
				"-dram:row_buffer_management_policy", length) == 0) {
			length = strlen(argv[argc_index + 1]);
			if (strncmp(argv[argc_index + 1], "open_page", length) == 0) {
				row_buffer_management_policy = OPEN_PAGE;
			} else if (strncmp(argv[argc_index + 1], "close_page", length) == 0) {
				row_buffer_management_policy = CLOSE_PAGE;
			} else {
				fprintf(stdout, "Unknown row buffer management policy. [%s]\n",
						argv[argc_index + 1]);
				exit(0);
			}
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:address_mapping_policy",
				length) == 0) {
			length = strlen(argv[argc_index + 1]);
			if (strncmp(argv[argc_index + 1], "burger_base_map", length) == 0) {
				address_mapping_policy = BURGER_BASE_MAP;
			} else if (strncmp(argv[argc_index + 1], "burger_alt_map", length)
					== 0) {
				address_mapping_policy = BURGER_ALT_MAP;
			} else if (strncmp(argv[argc_index + 1], "sdram_base_map", length)
					== 0) {
				address_mapping_policy = SDRAM_BASE_MAP;
			} else if (strncmp(argv[argc_index + 1], "sdram_hiperf_map", length)
					== 0) {
				address_mapping_policy = SDRAM_HIPERF_MAP;
			} else if (strncmp(argv[argc_index + 1], "intel845g_map", length)
					== 0) {
				address_mapping_policy = INTEL845G_MAP;
			} else if (strncmp(argv[argc_index + 1], "sdram_close_page_map",
					length) == 0) {
				address_mapping_policy = SDRAM_CLOSE_PAGE_MAP;
			} else {
				fprintf(stdout, "Unknown Address mapping policy. [%s]\n",
						argv[argc_index + 1]);
				exit(0);
			}
			argc_index += 2;

		} else if (strncmp(argv[argc_index], "-biu:trans_selection", length)
				== 0) {
			length = strlen(argv[argc_index + 1]);
			if (strncmp(argv[argc_index + 1], "fcfs", length) == 0) {
				config->set_transaction_selection_policy(FCFS);
			} else if (strncmp(argv[argc_index + 1], "riff", length) == 0) {
				config->set_transaction_selection_policy(RIFF);
			} else if (strncmp(argv[argc_index + 1], "hstp", length) == 0) {
				config->set_transaction_selection_policy(HSTP);
			} else if (strncmp(argv[argc_index + 1], "obf", length) == 0) {
				config->set_transaction_selection_policy(OBF);
			} else if (strncmp(argv[argc_index + 1], "wang", length) == 0) {
				config->set_transaction_selection_policy(WANG);
			} else if (strncmp(argv[argc_index + 1], "most_pending", length)
					== 0) {
				config->set_transaction_selection_policy(MOST_PENDING);
			} else if (strncmp(argv[argc_index + 1], "least_pending", length)
					== 0) {
				config->set_transaction_selection_policy(LEAST_PENDING);
			} else if (strncmp(argv[argc_index + 1], "greedy", length) == 0) {
				config->set_transaction_selection_policy(GREEDY);
			} else {
				fprintf(stdout, "Unknown Transaction Selection Type [%s]\n",
						argv[argc_index + 1]);
				exit(0);
			}
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:refresh_issue_policy",
				length) == 0) {
			length = strlen(argv[argc_index + 1]);
			if (strncmp(argv[argc_index + 1], "opportunistic", length) == 0) {
				refresh_issue_policy = REFRESH_OPPORTUNISTIC;
			} else if (strncmp(argv[argc_index + 1], "priority", length) == 0) {
				refresh_issue_policy = REFRESH_HIGHEST_PRIORITY;
			}
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:refresh_policy", length)
				== 0) {
			length = strlen(argv[argc_index + 1]);
			if (!strncmp(argv[argc_index + 1],
					"refresh_one_chan_all_rank_all_bank", length)) {
				config->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK);
			} else if (!strncmp(argv[argc_index + 1],
					"refresh_one_chan_all_rank_one_bank", length)) {
				config->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK);
			} else if (!strncmp(argv[argc_index + 1],
					"refresh_one_chan_one_rank_one_bank", length)) {
				config->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK);
			} else if (!strncmp(argv[argc_index + 1],
					"refresh_one_chan_one_rank_all_bank", length)) {
				config->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK);
			} else {
				fprintf(
						stderr,
						"\n\n\n\nExpecting refresh policy, found [%s] instead\n\n\n",
						argv[argc_index + 1]);
				config->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK);
			}
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-stat:biu", length) == 0) {
			biu_stats = true;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-stat:biu-slot", length) == 0) {
			biu_slot_stats = true;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-stat:biu-access-dist", length)
				== 0) {
			biu_access_dist_stats = true;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-stat:bundle", length) == 0) {
			bundle_stats = true;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-stat:tran_queue", length) == 0) {
			tran_queue_stats = true;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-stat:dram:bank_hit", length)
				== 0) {
			bank_hit_stats = true;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-stat:dram:bank_conflict", length)
				== 0) {
			bank_conflict_stats = true;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-stat:dram:cas_per_ras", length)
				== 0) {
			cas_per_ras_stats = true;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-stat:power", length) == 0) {
			power_stats_fileout = (char *) malloc(sizeof(char)
					* MAX_FILENAME_LENGTH);
			strcpy(power_stats_fileout, argv[argc_index + 1]);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-stat:all", length) == 0) {
			common_stats_fileout = (char *) malloc(sizeof(char)
					* MAX_FILENAME_LENGTH);
			strcpy(common_stats_fileout, argv[argc_index + 1]);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-dram:strict_ordering", length)
				== 0) {
			strict_ordering_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:biu", length) == 0) {
			biu_debug_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:transaction", length) == 0) {
			transaction_debug_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:dram", length) == 0) {
			dram_debug_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:wave", length) == 0) {
			wave_debug_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:cas", length) == 0) {
			cas_wave_debug_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:all", length) == 0) {
			all_debug_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:biu", length) == 0) {
			biu_debug_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:bundle", length) == 0) {
			bundle_debug_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:amb", length) == 0) {
			amb_buffer_debug_flag = TRUE;
			argc_index++;
		} else if (strncmp(argv[argc_index], "-debug:threshold", length) == 0) {
			sscanf(argv[argc_index + 1], "%u", &debug_tran_id_threshold);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-watch:trans", length) == 0) {
			sscanf(argv[argc_index + 1], "%d", &debug_tran_id);
			// Watch Transaction option
			config->set_tran_watch(debug_tran_id);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-watch:ref_trans", length) == 0) {
			sscanf(argv[argc_index + 1], "%d", &debug_ref_tran_id);
			printf("%d", debug_ref_tran_id);
			config->set_ref_tran_watch(debug_ref_tran_id);
			argc_index += 2;
		} else if (strncmp(argv[argc_index], "-arrivaldist:poisson", length)
				== 0) {
			arrival_distribution_model = POISSON_DISTRIBUTION;

			argc_index++;
		} else if (strncmp(argv[argc_index], "-arrivaldist:gaussian", length)
				== 0) {
			arrival_distribution_model = GAUSSIAN_DISTRIBUTION;

			argc_index++;
		} else if (strncmp(argv[argc_index], "-arrivaldist:uniform", length)
				== 0) {
			arrival_distribution_model = UNIFORM_DISTRIBUTION;

			argc_index++;
		} else if (strncmp(argv[argc_index], "-accessdist", length) == 0) {
			float access_distribution[4];
			sscanf(argv[argc_index + 1], "%e", &access_distribution[0]);
			sscanf(argv[argc_index + 2], "%e", &access_distribution[1]);
			sscanf(argv[argc_index + 3], "%e", &access_distribution[2]);
			sscanf(argv[argc_index + 4], "%e", &access_distribution[3]);

			argc_index += 5;
		} else {
			fprintf(stdout, "Unknown option [%s]\n", argv[argc_index]);
			exit(0);
		}
	}

	//defualt simulation = random
	//defualt model = uniform
	if (sim == NULL) { //no trace input was specified
		sim = new DRAM_Sim_RAND(biu, config, arrival_distribution_model,
				access_distribution);
	}

	/* Read the timing parameters in */
	if ((spd_filein[0] != '\0')) {
		if (((spd_fileptr = fopen(&spd_filein[0], "r+")) != NULL)) {
			read_dram_config_from_file(spd_fileptr, config);
		} else {
			fprintf(stdout,
					"ERROR: cannot read dram spd configuration file %s\n",
					spd_filein);
			exit(1);
		}
	}
	/* Read the power parameters in */
	if ((power_filein[0] != '\0')) {
		if ((power_fileptr = fopen(power_filein, "r+")) != NULL) {
			read_power_config_from_file(power_fileptr, sim->dram->power_config);
		}else{
			fprintf(stdout, "Error: cannot open power file %s\n", power_filein);
		}
	}
	/* Set up the power output if power_filein exists or name of power output
	 * is specified*/
	if (power_stats_fileout != NULL) {
		sim->dram->power_config->mem_initialize_power_collection(
				power_stats_fileout);
	}

	/* Finally, allow the command line switches to override defaults */
	if (dram_frequency != MEM_STATE_INVALID) {
		config->set_dram_frequency(dram_frequency); /* units in MHz */
	}
	if (channel_count != MEM_STATE_INVALID)
		config->set_dram_channel_count(channel_count); /* single channel of memory */

	if (cpu_frequency != MEM_STATE_INVALID)
		config->set_cpu_frequency(cpu_frequency); /* input value in MHz */
	if (dram_type != MEM_STATE_INVALID)
		config->set_dram_type(dram_type); /* SDRAM, DDRSDRAM,  */
	if (channel_width != MEM_STATE_INVALID)
		config->set_dram_channel_width(channel_width); /* data bus width, in bytes */

	dram_frequency = config->get_dram_frequency();
	memory_frequency = config->get_memory_frequency();
	config->set_cpu_memory_frequency_ratio(); /* finalize the ratio here */
	if (refresh_interval > 0) {
		config->enable_auto_refresh(TRUE, refresh_interval); /* set auto refresh, cycle through every "refresh_interval" ms */
	} else if (refresh_interval == 0) { /* Explicitly disable refresh interval **/
		config->enable_auto_refresh(FALSE, refresh_interval);
	}
	config->set_strict_ordering(strict_ordering_flag);
	if (address_mapping_policy != MEM_STATE_INVALID)
		config->set_pa_mapping_policy(address_mapping_policy);
	if (row_buffer_management_policy != MEM_STATE_INVALID)
		config->set_dram_row_buffer_management_policy(
				row_buffer_management_policy); /* OPEN_PAGE, CLOSED_PAGE etc */
	if (chipset_delay > 1) { /* someone overrode default, so let us set the delay for them */
		config->set_chipset_delay(chipset_delay);
	}

	biu->set_biu_delay(biu_delay);
	config->set_dram_refresh_issue_policy(refresh_issue_policy);
	// Check that wang transaction selection policy is only with close page -
	// refer to David Wangs Phd Thesis - rank hopping algorithm.
	if (config->get_transaction_selection_policy() == WANG
			&& (config->get_dram_row_buffer_management_policy() == OPEN_PAGE
					|| (config->get_dram_type() != DDR2
							&& config->get_dram_type() != DDR3))) {
		fprintf(
				stderr,
				" Warning: The Wang transaction selection policy is only for close page systems \n");
	}

	config->convert_config_dram_cycles_to_mem_cycles();

	biu->set_biu_debug(biu_debug_flag);
	config->set_dram_debug(dram_debug_flag);
	config->set_transaction_debug(transaction_debug_flag);
	config->set_biu_depth(bus_queue_depth);

	config->set_debug_tran_id_threshold(debug_tran_id_threshold);

	//
	config->set_wave_debug(wave_debug_flag);
	config->set_wave_cas_debug(cas_wave_debug_flag);
	config->set_bundle_debug(bundle_debug_flag);
	config->set_amb_buffer_debug(amb_buffer_debug_flag);

	if (all_debug_flag == TRUE) {
		biu->set_biu_debug(TRUE);
		config->set_dram_debug(TRUE);
		config->set_transaction_debug(TRUE);
		config->set_wave_debug(FALSE);
	} else if (wave_debug_flag == TRUE) {
		biu->set_biu_debug(FALSE);
		config->set_dram_debug(FALSE);
		//set_transaction_debug(FALSE);
		config->set_wave_debug(TRUE);
	}

	if (common_stats_fileout != NULL)
		sim->aux_stat->mem_stat_init_common_file(common_stats_fileout);
	if (biu_stats)
		sim->aux_stat->mem_gather_stat_init(GATHER_BUS_STAT, 0, 0);
	if (bundle_stats) {
		sim->aux_stat->mem_gather_stat_init(GATHER_BUNDLE_STAT, 0, 0);
		sim->aux_stat->init_extra_bundle_stat_collector();
	}
	if (tran_queue_stats)
		sim->aux_stat->mem_gather_stat_init(GATHER_TRAN_QUEUE_VALID_STAT, 0, 0);
	if (biu_slot_stats)
		sim->aux_stat->mem_gather_stat_init(GATHER_BIU_SLOT_VALID_STAT, 0, 0);
	if (biu_access_dist_stats)
		sim->aux_stat->mem_gather_stat_init(GATHER_BIU_ACCESS_DIST_STAT, 0, 0);

	if (bank_hit_stats)
		sim->aux_stat->mem_gather_stat_init(GATHER_BANK_HIT_STAT, 0, 0);
	if (bank_conflict_stats)
		sim->aux_stat->mem_gather_stat_init(GATHER_BANK_CONFLICT_STAT, 0, 0);
	if (cas_per_ras_stats)
		sim->aux_stat->mem_gather_stat_init(GATHER_CAS_PER_RAS_STAT, 0, 0);

	//config->dump_config(stdout);
	sim->max_cpu_time = max_inst; /* This is a hack to leverage the -max:inst switch here for cycle count */
	sim->current_cpu_time = 0;
	/*set_addr_debug(TRUE);*/

	return sim;

}

void DRAM_Sim::read_power_config_from_file(FILE * fp,
		PowerConfig * power_config_ptr) {
	char *ret;
	char line[512];
	if (fp == NULL) {
		fprintf(stderr, "Invalid Power Configuration File Pointer\n");
		exit(0);
	}
	//Read the DRAM characteristic from the file
	do {
		ret = fgets(line, sizeof(line) - 1, fp);
		line[sizeof(line) - 1] = '\0';
		int len = strlen(line);
		if (len > 0)
			line[len - 1] = '\0';
		if ((ret != NULL) && strlen(line)) {
			if (line[0] != '#') {
				//parse the parameters here!
				if (!strncmp(line, "density", 7)) {
					sscanf(line, "density %d", &power_config_ptr->density);
					if (power_config_ptr->density < 1)
						fprintf(stderr, "density must be greater than zero.");
				} else if (!strncmp(line, "chip_count", 10)) {
					sscanf(line, "chip_count %d", &power_config_ptr->chip_count);
					if (power_config_ptr->chip_count < 1)
						fprintf(stderr, "Chip Count must be greater than zero.");
				} else if (!strncmp(line, "DQS", 3)) {
					sscanf(line, "DQS %d", &power_config_ptr->DQS);
					if (power_config_ptr->DQS < 1)
						fprintf(stderr, "DQS must be greater than zero.");
				} else if (!strncmp(line, "max_VDD", 7)) {
					sscanf(line, "max_VDD %f", &power_config_ptr->max_VDD);
					if (power_config_ptr->max_VDD <= 0)
						fprintf(stderr, "max_VDD must be greater than zero.");
				} else if (!strncmp(line, "min_VDD", 7)) {
					sscanf(line, "min_VDD %f", &power_config_ptr->min_VDD);
					if ((power_config_ptr->min_VDD <= 0)
							|| (power_config_ptr->min_VDD
									> power_config_ptr->max_VDD))
						fprintf(stderr,
						"min_VDD must be greater than zero and smaller than max_VDD.");
				} else if (!strncmp(line, "IDD0", 4)) {
					sscanf(line, "IDD0 %d", &power_config_ptr->IDD0);
					if (power_config_ptr->IDD0 < 1)
						fprintf(stderr, "IDD0 must be greater than zero.");
				} else if (!strncmp(line, "IDD2P", 5)) {
					sscanf(line, "IDD2P %d", &power_config_ptr->IDD2P);
					if (power_config_ptr->IDD2P < 1)
						fprintf(stderr, "IDD2P must be greater than zero.");
				} else if (!strncmp(line, "IDD2F", 5)) {
					sscanf(line, "IDD2F %d", &power_config_ptr->IDD2F);
					if (power_config_ptr->IDD2F < 1)
						fprintf(stderr, "IDD2F must be greater than zero.");
				} else if (!strncmp(line, "IDD3P", 5)) {
					sscanf(line, "IDD3P %d", &power_config_ptr->IDD3P);
					if (power_config_ptr->IDD3P < 1)
						fprintf(stderr, "IDD3P must be greater than zero.");
				} else if (!strncmp(line, "IDD3N", 5)) {
					sscanf(line, "IDD3N %d", &power_config_ptr->IDD3N);
					if (power_config_ptr->IDD3N < 1)
						fprintf(stderr, "IDD3N must be greater than zero.");
				} else if (!strncmp(line, "IDD4R", 5)) {
					sscanf(line, "IDD4R %d", &power_config_ptr->IDD4R);
					if (power_config_ptr->IDD4R < 1)
						fprintf(stderr, "IDD4R must be greater than zero.");
				} else if (!strncmp(line, "IDD4W", 5)) {
					sscanf(line, "IDD4W %d", &power_config_ptr->IDD4W);
					if (power_config_ptr->IDD4W < 1)
						fprintf(stderr, "IDD4W must be greater than zero.");
				} else if (!strncmp(line, "IDD5", 4)) {
					sscanf(line, "IDD5 %d", &power_config_ptr->IDD5);
					if (power_config_ptr->IDD5 < 1)
						fprintf(stderr, "IDD5 must be greater than zero.");
				} else if (!strncmp(line, "t_CK", 4)) {
					sscanf(line, "t_CK %f", &power_config_ptr->t_CK);
					if (power_config_ptr->t_CK < 1.0)
						fprintf(stderr, "t_CK must be greater than zero.");
				} else if (!strncmp(line, "VDD", 3)) {
					sscanf(line, "VDD %f", &power_config_ptr->VDD);
					if (power_config_ptr->VDD <= 0)
						fprintf(stderr, "VDD must be greater than zero.");
				} else if (!strncmp(line, "P_per_DQ", 8)) {
					sscanf(line, "P_per_DQ %f", &power_config_ptr->P_per_DQ);
					if (power_config_ptr->P_per_DQ <= 0)
						fprintf(stderr, "P_per_DQ must be greater than zero.");
				} else if (!strncmp(line, "t_RFC_min", 9)) {
					sscanf(line, "t_RFC_min %f", &power_config_ptr->t_RFC_min);
					if (power_config_ptr->t_RFC_min <= 0)
						fprintf(stderr, "t_RFC_min must be greater than zero.");
				} else if (!strncmp(line, "t_REFI", 7)) {
					sscanf(line, "t_REFI %f", &power_config_ptr->t_REFI);
					if (power_config_ptr->P_per_DQ <= 0)
						fprintf(stderr, "t_REFI must be greater than zero.");
				} else if (!strncmp(line, "ICC_Idle_2", 10)) {
					sscanf(line, "ICC_Idle_2 %f", &power_config_ptr->ICC_Idle_2);
					if (power_config_ptr->ICC_Idle_2 <= 0)
						fprintf(stderr, "ICC_Idle_2 must be greater than zero.");
				} else if (!strncmp(line, "ICC_Idle_1", 10)) {
					sscanf(line, "ICC_Idle_1 %f", &power_config_ptr->ICC_Idle_1);
					if (power_config_ptr->ICC_Idle_1 <= 0)
						fprintf(stderr, "ICC_Idle_1 must be greater than zero.");
				} else if (!strncmp(line, "ICC_Idle_0", 10)) {
					sscanf(line, "ICC_Idle_0 %f", &power_config_ptr->ICC_Idle_0);
					if (power_config_ptr->ICC_Idle_0 <= 0)
						fprintf(stderr, "ICC_Idle_0 must be greater than zero.");
				} else if (!strncmp(line, "ICC_Active_1", 12)) { /* Active */
					sscanf(line, "ICC_Active_1 %f",
							&power_config_ptr->ICC_Active_1);
					if (power_config_ptr->ICC_Active_1 <= 0)
						fprintf(stderr,
						"ICC_Active_1 must be greater than zero.");
				} else if (!strncmp(line, "ICC_Active_2", 12)) { /* Data Pass through */
					sscanf(line, "ICC_Active_2 %f",
							&power_config_ptr->ICC_Active_2);
					if (power_config_ptr->ICC_Active_2 <= 0)
						fprintf(stderr,
						"ICC_Active_2 must be greater than zero.");
				} else if (!strncmp(line, "VCC", 3)) { /* AMB Voltage */
					sscanf(line, "VCC %f", &power_config_ptr->VCC);
					if (power_config_ptr->VCC <= 0)
						fprintf(stderr, "VCC must be greater than zero.");
				}

			}
		}
	} while (!feof(fp));

	//  calculate_power_values(power_config_ptr);

}

void DRAM_Sim::read_dram_config_from_file(FILE *fin, DRAM_config *this_c) {
	char c;
	char input_string[256];
	int input_int;
	int dram_config_token;

	while ((c = fgetc(fin)) != EOF) {
		if ((c != EOL) && (c != CR) && (c != SPACE) && (c != TAB)) {
			fscanf(fin, "%s", &input_string[1]);
			input_string[0] = c;
		} else {
			fscanf(fin, "%s", &input_string[0]);
		}
		dram_config_token = file_io_token(&input_string[0]);
		switch (dram_config_token) {
		case dram_type_token:
			fscanf(fin, "%s", &input_string[0]);
			if (!strncmp(input_string, "sdram", 5)) {
				this_c->set_dram_type(SDRAM); /* SDRAM, DDRSDRAM,  etc */
			} else if (!strncmp(input_string, "ddrsdram", 8)) {
				this_c->set_dram_type(DDRSDRAM);
			} else if (!strncmp(input_string, "ddr2", 4)) {
				this_c->set_dram_type(DDR2);
			} else if (!strncmp(input_string, "ddr3", 4)) {
				this_c->set_dram_type(DDR3);
			} else if (!strncmp(input_string, "fbd_ddr2", 8)) {
				this_c->set_dram_type(FBD_DDR2);
			} else {
				this_c->set_dram_type(SDRAM);
			}
			break;
		case data_rate_token: /* aka memory_frequency: units is MBPS */
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_frequency(input_int);
			break;
		case dram_clock_granularity_token:
			fscanf(fin, "%d", &(this_c->dram_clock_granularity));
			break;
		case critical_word_token:
			fscanf(fin, "%s", &input_string[0]);
			if (!strncmp(input_string, "TRUE", 4)) {
				this_c->critical_word_first_flag = TRUE;
			} else if (!strncmp(input_string, "FALSE", 5)) {
				this_c->critical_word_first_flag = FALSE;
			} else {
				fprintf(
						stderr,
						"\n\n\n\nCritical Word First Flag, Expecting TRUE or FALSE, found [%s] instead\n\n\n",
						input_string);
			}
			break;
		case PA_mapping_policy_token:
			fscanf(fin, "%s", &input_string[0]);
			if (!strncmp(input_string, "burger_base_map", 15)) {
				this_c->set_pa_mapping_policy(BURGER_BASE_MAP); /* How to map physical address to memory address */
			} else if (!strncmp(input_string, "burger_alt_map", 14)) {
				this_c->set_pa_mapping_policy(BURGER_ALT_MAP);
			} else if (!strncmp(input_string, "sdram_base_map", 14)) {
				this_c->set_pa_mapping_policy(SDRAM_BASE_MAP);
			} else if (!strncmp(input_string, "sdram_hiperf_map", 16)) {
				this_c->set_pa_mapping_policy(SDRAM_HIPERF_MAP);
			} else if (!strncmp(input_string, "intel845g_map", 13)) {
				this_c->set_pa_mapping_policy(INTEL845G_MAP);
			} else if (!strncmp(input_string, "sdram_close_page_map", 20)) {
				this_c->set_pa_mapping_policy(SDRAM_CLOSE_PAGE_MAP);
			} else {
				fprintf(
						stderr,
						"\n\n\n\nExpecting mapping policy, found [%s] instead\n\n\n",
						input_string);
				this_c->set_pa_mapping_policy(SDRAM_BASE_MAP);
			}
			break;
		case row_buffer_management_policy_token:
			fscanf(fin, "%s", &input_string[0]);
			if (!strncmp(input_string, "open_page", 9)) {
				this_c->set_dram_row_buffer_management_policy(OPEN_PAGE); /* OPEN_PAGE, CLOSED_PAGE etc */
			} else if (!strncmp(input_string, "close_page", 10)) {
				this_c->set_dram_row_buffer_management_policy(CLOSE_PAGE);
			} else if (!strncmp(input_string, "perfect_page", 12)) {
				this_c->set_dram_row_buffer_management_policy(PERFECT_PAGE);
			} else {
				fprintf(
						stderr,
						"\n\n\n\nExpecting buffer management policy, found [%s] instead\n\n\n",
						input_string);
				this_c->set_dram_row_buffer_management_policy(OPEN_PAGE);
			}
			break;
		case cacheline_size_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_transaction_granularity(input_int);
			break;
		case channel_count_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_channel_count(input_int);
			break;
		case channel_width_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_channel_width(input_int);
			break;
		case rank_count_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_rank_count(input_int);
			break;
		case bank_count_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_bank_count(input_int);
			break;
		case row_count_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_row_count(input_int);
			break;
		case col_count_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_col_count(input_int);
			break;
		case t_cas_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_cas = input_int;
			break;
		case t_cmd_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_cmd = input_int;
			break;
		case t_cwd_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_cwd = input_int;
			break;
		case t_dqs_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_dqs = input_int;
			break;
		case t_faw_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_faw = input_int;
			break;
		case t_ras_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_ras = input_int;
			break;
		case t_rcd_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_rcd = input_int;
			break;
		case t_rc_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_rc = input_int;
			break;
		case t_rfc_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_rfc = input_int;
			break;
		case t_rrd_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_rrd = input_int;
			break;
		case t_rp_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_rp = input_int;
			break;
		case t_wr_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_wr = input_int;
			break;
		case t_rtp_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_rtp = input_int;
			break;
		case posted_cas_token:
			fscanf(fin, "%s", &input_string[0]);
			if (!strncmp(input_string, "TRUE", 4)) {
				this_c->posted_cas_flag = TRUE;
			} else if (!strncmp(input_string, "FALSE", 5)) {
				this_c->posted_cas_flag = FALSE;
			} else {
				fprintf(
						stderr,
						"\n\n\n\nPosted CAS Flag, Expecting TRUE or FALSE, found [%s] instead\n\n\n",
						input_string);
			}
			break;
		case t_al_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_al = input_int;
			break;
		case t_rl_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_rl = input_int;
			break;
		case t_cac_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_cac = input_int;
			break;
		case t_rtr_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_rtr = input_int;
			break;
		case auto_refresh_token:
			fscanf(fin, "%s", &input_string[0]);
			if (!strncmp(input_string, "TRUE", 4)) {
				this_c->auto_refresh_enabled = TRUE;
			} else if (!strncmp(input_string, "FALSE", 5)) {
				this_c->auto_refresh_enabled = FALSE;
			} else {
				fprintf(
						stderr,
						"\n\n\n\nAuto Refresh Enable, Expecting TRUE or FALSE, found [%s] instead\n\n\n",
						input_string);
			}

			break;
		case auto_refresh_policy_token:
			fscanf(fin, "%s", &input_string[0]);
			if (!strncmp(input_string, "refresh_one_chan_all_rank_all_bank", 34)) {
				this_c->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK);
			} else if (!strncmp(input_string,
					"refresh_one_chan_all_rank_one_bank", 34)) {
				this_c->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK);
			} else if (!strncmp(input_string,
					"refresh_one_chan_one_rank_one_bank", 34)) {
				this_c->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK);
			} else if (!strncmp(input_string,
					"refresh_one_chan_one_rank_all_bank", 34)) {
				this_c->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK);
			} else {
				fprintf(
						stderr,
						"\n\n\n\nExpecting refresh policy, found [%s] instead\n\n\n",
						input_string);
				this_c->set_dram_refresh_policy(
						REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK);
			}

			break;
		case refresh_time_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_refresh_time(input_int);
			/* Note: Time is specified in us */
			break;
		case comment_token:
			while (((c = fgetc(fin)) != EOL) && (c != EOF)) {
				/*comment, to be ignored */
			}
			break;
			/**** FB-DIMM tokens **/
		case t_bus_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_bus = input_int; //this value is already in MC bus time
			break;
		case var_latency_token:
			fscanf(fin, "%s", &input_string[0]);
			if (!strncmp(input_string, "TRUE", 4)) {
				this_c->var_latency_flag = TRUE;
			} else if (!strncmp(input_string, "FALSE", 5)) {
				this_c->var_latency_flag = FALSE;
			} else {
				fprintf(
						stderr,
						"\n\n\n\nVariable Latency flag, Expecting TRUE or FALSE, found [%s] instead\n\n\n",
						input_string);
			}
			break;

		case up_channel_width_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_up_channel_width(input_int);
			break;
		case down_channel_width_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_down_channel_width(input_int);
			break;
		case t_amb_up_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_amb_up = input_int;
			break;
		case t_amb_down_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_amb_down = input_int;
			break;
		case t_bundle_token:
			fscanf(fin, "%d", &input_int);
			this_c->t_bundle = input_int;
			break;
		case mem_cont_freq_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_memory_frequency(input_int);
			break;
		case dram_freq_token:
			fscanf(fin, "%d", &input_int);
			this_c->set_dram_frequency(input_int);
			break;
		case amb_up_buffer_token:
			fscanf(fin, "%d", &input_int);
			this_c->up_buffer_cnt = input_int;
			break;
		case amb_down_buffer_token:
			fscanf(fin, "%d", &input_int);
			this_c->down_buffer_cnt = input_int;
			break;
		default:
		case unknown_token:
			fprintf(stderr, "Unknown Token [%s]\n", input_string);
			break;
		}
	}
}

int DRAM_Sim::file_io_token(char *input) {
	size_t length;
	length = strlen(input);
	if (strncmp(input, "//", 2) == 0) {
		return comment_token;
	} else if (strncmp(input, "type", length) == 0) {
		return dram_type_token;
	} else if (strncmp(input, "datarate", length) == 0) {
		return data_rate_token;
	} else if (strncmp(input, "clock_granularity", length) == 0) {
		return dram_clock_granularity_token;
	} else if (strncmp(input, "critical_word_first", length) == 0) {
		return critical_word_token;
	} else if (strncmp(input, "VA_mapping_policy", length) == 0) {
		return VA_mapping_policy_token;
	} else if (strncmp(input, "PA_mapping_policy", length) == 0) {
		return PA_mapping_policy_token;
	} else if (strncmp(input, "row_buffer_policy", length) == 0) {
		return row_buffer_management_policy_token;
	} else if (strncmp(input, "cacheline_size", length) == 0) {
		return cacheline_size_token;
	} else if (strncmp(input, "channel_count", length) == 0) {
		return channel_count_token;
	} else if (strncmp(input, "channel_width", length) == 0) {
		return channel_width_token;
	} else if (strncmp(input, "rank_count", length) == 0) {
		return rank_count_token;
	} else if (strncmp(input, "bank_count", length) == 0) {
		return bank_count_token;
	} else if (strncmp(input, "row_count", length) == 0) {
		return row_count_token;
	} else if (strncmp(input, "col_count", length) == 0) {
		return col_count_token;
	} else if (strncmp(input, "t_cas", length) == 0) {
		return t_cas_token;
	} else if (strncmp(input, "t_cmd", length) == 0) {
		return t_cmd_token;
	} else if (strncmp(input, "t_cwd", length) == 0) {
		return t_cwd_token;
	} else if (strncmp(input, "t_cac", length) == 0) {
		return t_cac_token;
	} else if (strncmp(input, "t_dqs", length) == 0) {
		return t_dqs_token;
	} else if (strncmp(input, "t_faw", length) == 0) {
		return t_faw_token;
	} else if (strncmp(input, "t_ras", length) == 0) {
		return t_ras_token;
	} else if (strncmp(input, "t_rcd", 5) == 0) {
		return t_rcd_token;
	} else if (strncmp(input, "t_rc", 4) == 0) {
		return t_rc_token;
	} else if (strncmp(input, "t_rrd", length) == 0) {
		return t_rrd_token;
	} else if (strncmp(input, "t_rp", length) == 0) {
		return t_rp_token;
	} else if (strncmp(input, "t_rfc", length) == 0) {
		return t_rfc_token;
	} else if (strncmp(input, "t_cac", length) == 0) {
		return t_cac_token;
	} else if (strncmp(input, "t_cwd", length) == 0) {
		return t_cwd_token;
	} else if (strncmp(input, "t_rtr", length) == 0) {
		return t_rtr_token;
	} else if (strncmp(input, "t_dqs", length) == 0) {
		return t_dqs_token;
	} else if (strncmp(input, "posted_cas", length) == 0) {
		return posted_cas_token;
	} else if (strncmp(input, "t_al", length) == 0) {
		return t_al_token;
	} else if (strncmp(input, "t_rl", length) == 0) {
		return t_rl_token;
	} else if (strncmp(input, "t_wr", length) == 0) {
		return t_wr_token;
	} else if (strncmp(input, "t_rtp", length) == 0) {
		return t_rtp_token;
	} else if (strncmp(input, "auto_refresh", length) == 0) {
		return auto_refresh_token;
	} else if (strncmp(input, "auto_refresh_policy", length) == 0) {
		return auto_refresh_policy_token;
	} else if (strncmp(input, "refresh_time", length) == 0) {
		return refresh_time_token;

		/******** FB-DIMM specific tokens ****/
	} else if (strncmp(input, "t_bus", length) == 0) {
		return t_bus_token;
	} else if (strncmp(input, "up_channel_width", length) == 0) {
		return up_channel_width_token;
	} else if (strncmp(input, "down_channel_width", length) == 0) {
		return down_channel_width_token;
	} else if (strncmp(input, "t_amb_up", length) == 0) {
		return t_amb_up_token;
	} else if (strncmp(input, "t_amb_down", length) == 0) {
		return t_amb_down_token;
	} else if (strncmp(input, "t_bundle", length) == 0) {
		return t_bundle_token;
	} else if (strncmp(input, "t_bus", length) == 0) {
		return t_bus_token;
	} else if (strncmp(input, "var_latency", length) == 0) {
		return var_latency_token;
	} else if (strncmp(input, "mc_freq", length) == 0) {
		return mem_cont_freq_token;
	} else if (strncmp(input, "dram_freq", length) == 0) {
		return dram_freq_token;
	} else if (strncmp(input, "amb_up_buffer_count", length) == 0) {
		return amb_up_buffer_token;
	} else if (strncmp(input, "amb_down_buffer_count", length) == 0) {
		return amb_down_buffer_token;
	} else {
		fprintf(stderr, " Unknown token %s %d\n", input, strncmp(input,
				"var_latency", length));
		return unknown_token;
	}
}

