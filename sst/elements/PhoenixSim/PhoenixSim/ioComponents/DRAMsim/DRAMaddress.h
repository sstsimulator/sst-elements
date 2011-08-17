#ifndef ADDRESS__H
#define ADDRESS__H

#include <stdint.h>

#include "DRAM_config.h"


class DRAMaddress {

public:

	DRAMaddress();

	uint64_t	virtual_address;	
	int		thread_id;
	uint64_t	physical_address;
	int		chan_id;		/* logical channel id */
	int		rank_id;		/* device id */
	int		bank_id;
	int		row_id;
	int		col_id;			/* column address */


	int convert_address(int , DRAM_config *);

	/*Redefined to take care of actual log2 math.h definitions */
	int  sim_dram_log2(int input);

	void print_address();
};

#endif
