#ifndef RBRR__H
#define RBRR__H

#include "constants.h"

class RBRR {

public:

	RBRR(){};

 int last_cmd; /* refers to the last CAS/RAS/PRE executed */
  int last_ref; /* If we executed a refresh we dont want to disturb the sequence */

  /* Stuff we need to execute now */
  int current_cmd; /* What we should execute now */
  /* RAS -> 0 CAS -> 1 PRE -> 2 REF -> 3 */
  int current_bank[4][MAX_RANK_COUNT];
  int current_rank[4];
  int cmd2indmapping[4]; 



	int get_rbrr_cmd_index();
	void set_rbrr_next_command(int nextRBRR_cmd);

};


#endif


