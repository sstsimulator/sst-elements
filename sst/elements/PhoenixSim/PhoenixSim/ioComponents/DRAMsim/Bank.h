#ifndef BANK__H
#define BANK__H


class Bank {

public:

	Bank(){ ras_done_time = 0;
			rcd_done_time = 0;
			cas_done_time = 0;
			rp_done_time = 0;
			rfc_done_time = 0;
			rtr_done_time = 0;
			twr_done_time = 0;
			cas_count_since_ras = 0;
	};


	int		status;			/* IDLE, ACTIVATING, ACTIVE, PRECHARGING */
	int		row_id;			/* if the bank is open to a certain row, what is the row id? */
	tick_t		ras_done_time;		/* precharge command can be accepted when this counter expires */
	tick_t		rcd_done_time;		/* cas command can be accepted when this counter expires */
	tick_t		cas_done_time;		/* When this command completes, data appears on the data bus */
	tick_t		rp_done_time;		/* another ras command can be accepted when this counter expires */
	tick_t		rfc_done_time;		/* another refresh/ras command can be accepted when this counter expires */
	tick_t		rtr_done_time;		/* for architecture with write buffer, this command means that a retirement command can be issued */
	tick_t		twr_done_time;		/* twr_done_time */
	int		last_command;		/* used to gather stats */
	int		cas_count_since_ras;	/* how many CAS have we done since the RAS? */
	uint64_t		bank_conflict;	/* how many CAS have we done since the RAS? */
	uint64_t		bank_hit;	/* how many CAS have we done since the RAS? */
	int 		paging_policy;		/* open or closed, per bank, used in dynamic policy */



};

#endif



