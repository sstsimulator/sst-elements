#ifndef CONSTANTS__H
#define CONSTANTS__H

#include <stdint.h>


#define DEBUG 1
#define MEMORY_SYSTEM_H	9763099
//#define DEBUG_POWER

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define	FALSE				0
#define TRUE				1

#define MAX_CPU_FREQUENCY		1000000		/* one teraherz */
#define MIN_CPU_FREQUENCY		10		/* ten MHz */
#define MAX_DRAM_FREQUENCY		1000000		/* one teraherz */
#define MIN_DRAM_FREQUENCY		10		/* ten MHz */
#define MIN_CHANNEL_WIDTH		2		/* narrowest allowable DRAM channel assumed to be 2 bytes */
#define MAX_CHANNEL_COUNT		8		/* max number of logical memory channels */
#define	MAX_RANK_COUNT			32		/* Number of ranks on a channel. A rank is a device-bank */
#define	MAX_BANK_COUNT			32		/* number of banks per rank (chip) */
#define MIN_CACHE_LINE_SIZE		32		/* min cacheline length assumed to be 32 bytes */

#define	MAX_AMB_BUFFER_COUNT	8		/* number of amb buffers */

/* There are 5 states to a memory transaction request
 *
 *  INVALID   - Entry is not being used
 *  VALID     - ENTRY has valid memory transaction request, but not yet scheduled by DRAM controller
 *  SCHEDULED - request has been accepted by the dram system, broken up into commands, and scheduled for execution in dram system
 *  ISSUED	- 	at least one of the commands associated with the request has been issued to the dram system.
 *  CRITICAL_WORD_RECEIVED - We can allow the callback function to run, since the data has returned.
 *  COMPLETED - request is complete, latency has been computed by dram system.
 */

#define MEM_STATE_INVALID					-1
#define MEM_STATE_VALID						1
#define MEM_STATE_SCHEDULED					2
#define	MEM_STATE_CRITICAL_WORD_RECEIVED	3
#define MEM_STATE_COMPLETED					4
#define MEM_STATE_ISSUED					5
#define MEM_STATE_DATA_ISSUED				6

#define UNKNOWN_RETRY			-2      	/* can't get a queue, so retry the whole thing */
#define UNKNOWN_PENDING			-1      	/* got a biu slot, just wait for DRAM return. */

#define	MEMORY_ACCESS_TYPES_COUNT	6		/* 5 different ways of accessing memory */
#define	MEMORY_UNKNOWN_COMMAND		0
#define	MEMORY_IFETCH_COMMAND		1
#define	MEMORY_WRITE_COMMAND		2
#define	MEMORY_READ_COMMAND		3
#define	MEMORY_PREFETCH			4

#define	MAX_BUS_QUEUE_DEPTH		1024 /* assume max 256 entry bus queue */
#define MAX_BUDDY_COUNT         	4
#define	MAX_ACCESS_HISTORY_TABLE_SIZE	4 * 1024 * 1024	/* 256 MB, 64 byte lines = 4 M entries */
#define RECENT_ACCESS_HISTORY_DEPTH 	4              /* housekeeping, keep track of address of last 4 fetched lines */

							/* Transaction prioritization policy */
#define	FCFS				0		/* default, First Come First Served */
#define	RIFF				1		/* Read or Instruction Fetch First */
#define RIFFWWS				2		/* Read or Instruction Fetch First with Write Sweeping */
#define HSTP                            3               /* higest priority first (for prefetching) */
#define OBF				4		/* DRAM tries to service requests to open bank first */
#define WANG				6		/* rank and bank round robin, through each rank and bank */
#define MOST_PENDING		7 		/* Rixners most pending scheme - issue cmds to rows with most transactions */
#define LEAST_PENDING		8 		/* Rixners most pending scheme - issue cmds to rows with most transactions */
#define GREEDY 				9
/* pfcache #def's */

#define	MAX_PREFETCH_CACHE_SIZE		256		/* 256 fully associative entries */
#define	NO_PREFETCH			0
#define	STREAM_PREFETCH			1
#define	MARKOV_MEMORY_PREFETCH		2		/* In memory storage of Morkov prefetch hints */
#define	MARKOV_TABLE_PREFETCH		3		/* on CPU Table of Morkov prefetch hints */

#define	UNKNOWN				-1
#define SDRAM				0
#define	DDRSDRAM			1
#define DDR2				2
#define FBD_DDR2			3			/*** FB-DIMM + DDR2 combination ***/
#define DDR3				4

/* these are #def's for use that tells the status of commands. INVALID assumed to be -1 */
/* A command is assumed to be IN_QUEUE, or in COMMAND_TRANSMISSION from controller to DRAM, or in
   PROCESSING by the DRAM ranks, or in DATA_TRANSMISSION back to the controller, or COMPLETED */

#define	MAX_TRANSACTION_QUEUE_DEPTH	64
#define	MIN_TRANSACTION_QUEUE_DEPTH	64 /* This is what the base config is */
#define	MAX_REFRESH_QUEUE_DEPTH		8		/* Number of Refreshes that you can hold upto */

#define IN_QUEUE			0
#define	LINK_COMMAND_TRANSMISSION	1  // COMMAND_TRANSMISSION
#define	DRAM_PROCESSING				2  // PROCESSING
#define LINK_DATA_TRANSMISSION	    3  // DATA_TRANSMISSION
#define	AMB_PROCESSING				5  // Left it as is, it technicall is AMB UP processing (except for DRIVE)
#define	DIMM_COMMAND_TRANSMISSION	6  // AMB COMMAND TRANSMISSION
#define DIMM_DATA_TRANSMISSION		7  // AMB_DATA_TRANSMISSION
#define AMB_DOWN_PROCESSING         8
#define DIMM_PRECHARGING			9 // Used only for CAS_WITH_PREC commands

/*  Already defined above
#define	COMPLETED			4
 */

/* These are #def's for DRAM "action" messages */
/* SCHEDULED 				2 */
/* COMPLETED 				4 */
#define XMIT2PROC			5
#define PROC2DATA			6
#define XMIT2AMBPROC			7
#define AMBPROC2AMBCT			8
#define AMBCT2PROC			9
#define AMBCT2DATA			10
#define AMBPROC2DATA			11
#define AMBPROC2PROC			12
#define DATA2AMBDOWN			13
#define AMBDOWN2DATA			14
#define LINKDATA2PREC			15

#define MAX_DEBUG_STRING_SIZE 	24000
#define MAX_TRANSACTION_LATENCY	10000

/* These are the bundle types */
#define THREE_COMMAND_BUNDLE		0
#define COMMAND_PLUS_DATA_BUNDLE	1

/*** Bundle stuff ***/
#define DATA_BYTES_PER_READ_BUNDLE		16		/** Number of data bytes in a read bundle **/
#define DATA_BYTES_PER_WRITE_BUNDLE		8		/** Number of data bytes in a write bundle **/
/****************** DRIVE DATA positions  ***************
 * *****************************************************/
#define DATA_FIRST			0
#define DATA_MIDDLE			1
#define DATA_LAST			2

#define BUNDLE_SIZE					3
#define DATA_CMD_SIZE				2		/** Occupies two slots in a bundle **/
/* these are #def's for use with commands and transactions INVALID assumed to be -1 */


/* these are #def's for use with commands and transactions INVALID assumed to be -1 */
#define NUM_COMMANDS		14
#define	RAS					1
#define	CAS					2
#define	CAS_AND_PRECHARGE	3
#define	CAS_WRITE				4		/* Write Command */
#define	CAS_WRITE_AND_PRECHARGE	5
#define RETIRE				6		/* Not currenly used */
#define	PRECHARGE			7
#define	PRECHARGE_ALL		8		/* precharge all, used as part of refresh */
#define	RAS_ALL				9		/* row access all.  Used as part of refresh */
#define	DRIVE				10		/* FB-DIMM -> data driven out of amb */
#define	DATA				11		/* FB-DIMM -> data being sent following a CAS to the DRAM */
#define CAS_WITH_DRIVE      12
#define	REFRESH				13		/* Refresh command  */
#define	REFRESH_ALL			14		/* Refresh command  */

/* Assumed state transitions of banks for each command*/

/*     PRECHARGE : (ACTIVE) -> PRECHARGING -> IDLE
 *     RAS  : (IDLE) -> ACTIVATING -> ACTIVE
 */

#define	BUSY				1

#define	IDLE 				-1
#define	PRECHARGING			1
#define	ACTIVATING			2
#define	ACTIVE				4

#define	BUS_BUSY			1
#define	BUS_TURNAROUND			2

/* This section defines the virtual address mapping policy */
#define VA_EQUATE			0			/* physical == virtual */
#define VA_SEQUENTIAL			1			/* sequential physical page assignment */
#define VA_RANDOM			2
#define VA_PER_BANK			3
#define VA_PER_RANK			4

/* This section defines the physical address mapping policy */

#define	BURGER_BASE_MAP			0
#define	BURGER_ALT_MAP			1
#define	INTEL845G_MAP			2
#define	SDRAM_BASE_MAP			3
#define SDRAM_HIPERF_MAP		4
#define SDRAM_CLOSE_PAGE_MAP		5


#define ADDRESS_MAPPING_FAILURE		0
#define ADDRESS_MAPPING_SUCCESS		1

/* This section defines the row buffer management policy */

#define OPEN_PAGE               	0
#define CLOSE_PAGE              	1
#define	PERFECT_PAGE			2

/* Refresh policies that are supported and should be supported **/
#define REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK		0		/** Initial policy supported by ras_all/pre_all -> whole system goes down for refresh*/
#define REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK	1		/* Only one bank in all ranks goes down for refresh  Currently not supported*/
#define REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK	2		/* Only one bank in one rank at a time goes down for refresh */
#define REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK	3		/* Standard Refresh policy available*/

/* When do we issue a refresh -> highest priority opportunistic */
#define	REFRESH_HIGHEST_PRIORITY	0
#define REFRESH_OPPORTUNISTIC		1

// Update the latency stat names structure in mem-stat.c when updating latency
// types
#define LATENCY_OVERHEAD_COUNT  13
#define REFRESH_CMD_CONFLICT_OVERHEAD   0
#define LINK_UP_BUS_BUSY_OVERHEAD  1
#define DIMM_BUS_BUSY_OVERHEAD  2
#define PROCESSING_OVERHEAD     3
#define BANK_CONFLICT_OVERHEAD  4
#define AMB_BUSY_OVERHEAD       5
#define LINK_DOWN_BUS_BUSY_OVERHEAD  6
#define STRICT_ORDERING_OVERHEAD	7
#define RAW_OVERHEAD		8
#define	MISC_X_OVERHEAD		9
#define BUNDLE_CONFLICT_OVERHEAD 10
#define PREV_TRAN_SCHEDULED_OVERHEAD 11
#define BIU_RETURN_OVERHEAD 12
/* This section used for statistics gathering */

#define	GATHER_BUS_STAT			0
#define	GATHER_BANK_HIT_STAT		1
#define	GATHER_BANK_CONFLICT_STAT	2
#define	GATHER_CAS_PER_RAS_STAT		3
#define GATHER_REQUEST_LOCALITY_STAT	4
#define GATHER_BUNDLE_STAT		5
#define GATHER_TRAN_QUEUE_VALID_STAT		6
#define GATHER_BIU_SLOT_VALID_STAT	7
#define GATHER_BIU_ACCESS_DIST_STAT	8

#define CMS_MAX_LOCALITY_RANGE		257             /* 256 + 1 */

/*
 * DEfine which trace this is.
 */

#define         NO_TRACE                0
#define         K6_TRACE                1
#define         TM_TRACE                2
#define         SS_TRACE                3
#define         MASE_TRACE              4


#define MAX_FILENAME_LENGTH	1024
#define UNIFORM_DISTRIBUTION 1
#define GAUSSIAN_DISTRIBUTION 2
#define POISSON_DISTRIBUTION 3

#ifndef PI
#define PI 3.141592654
#endif

#define MAX_TRACES 8


#define MAX_PROCESSOR 32

typedef unsigned long long 	tick_t;
//typedef uint64_t tick_t;

typedef void (*RELEASE_FN_TYPE)(unsigned int rid,int lat);

#define 	EOL 	10
#define 	CR 	13
#define 	SPACE	32
#define		TAB	9

enum spd_token_t {
	dram_type_token = 0,
	data_rate_token,
	dram_clock_granularity_token,
	critical_word_token,
	VA_mapping_policy_token,
	PA_mapping_policy_token,
	row_buffer_management_policy_token,
	auto_precharge_interval_token,
	cacheline_size_token,
	channel_count_token,
	channel_width_token,
	rank_count_token,
	bank_count_token,
	row_count_token,
	col_count_token,
	t_cas_token,
	t_cmd_token,
	t_cwd_token,
	t_dqs_token,
	t_faw_token,
	t_ras_token,
	t_rc_token,
	t_rcd_token,
	t_rrd_token,
	t_rp_token,
	t_rfc_token,
	t_wr_token,
	t_rtp_token,
	t_rtr_token,
	t_cac_token,
	posted_cas_token,
	t_al_token,
	t_rl_token,
	auto_refresh_token,
	auto_refresh_policy_token,
	refresh_time_token,
	comment_token,
	unknown_token,
	EOL_token,
	/************ FB-DIMM tokens ******/
	t_bus_token,
	var_latency_token,
	up_channel_width_token,
	down_channel_width_token,
	t_amb_up_token,
	t_amb_down_token,
	t_bundle_token,
	mem_cont_freq_token,
	dram_freq_token,
	amb_up_buffer_token,
	amb_down_buffer_token
};



#endif
