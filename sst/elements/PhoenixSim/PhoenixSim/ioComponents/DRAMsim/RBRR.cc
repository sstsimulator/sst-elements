#include "RBRR.h"



int RBRR::get_rbrr_cmd_index() {
  int index = -1;
  if (current_cmd == RAS) 
	index = 0;
  else if (current_cmd == CAS)
	index = 1;
  else if (current_cmd == PRECHARGE)
	index = 2;
  else if (current_cmd == REFRESH)
	index = 3;
  return index;
}


void RBRR::set_rbrr_next_command(int nextRBRR_cmd) {
  if (nextRBRR_cmd == RAS) 
	 current_cmd = CAS;
  else if (nextRBRR_cmd == CAS)
	current_cmd = PRECHARGE;
  else if (nextRBRR_cmd == PRECHARGE)
	 current_cmd = RAS;
}


