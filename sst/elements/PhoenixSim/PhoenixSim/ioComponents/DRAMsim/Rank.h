#ifndef RANK__H
#define RANK__H

#include "Bank.h"
#include "DIMM.h"
#include "PowerCounter.h"


class Rank {

public:
	Rank(){};


  Bank		bank[MAX_BANK_COUNT];
  tick_t		activation_history[4];	/* keep track of time of last four activations for tFAW and tRRD*/
  //Ohm--Adding for power calculation
  PowerCounter	r_p_info; /* Global Access Snapshot */
  PowerCounter	r_p_gblinfo; /* Global Access Snapshot */
  bool			cke_bit;		/* Set to 1 if the rank CKE is enabled */
  uint64_t	cke_tid;  	/* Transaction which caused it to be set high */
  uint64_t	cke_cmd;  	/* Transaction which caused it to be set high */
  tick_t		cke_done;
  DIMM		*my_dimm;  



};

#endif

