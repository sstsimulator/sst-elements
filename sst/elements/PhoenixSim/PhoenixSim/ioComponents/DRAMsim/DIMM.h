#ifndef DIMM__H
#define DIMM__H


#include "DRAM_config.h"

class DIMM {

public:

	DIMM();


 int			up_buffer_cnt; /* Buffer size : amt of data sent per bundle default 8 butes*/
  int			down_buffer_cnt; /* Buffer size : amt of data sent out per bundle default 16 butes*/
  //AMB_Buffer	*up_buffer;
  //AMB_Buffer	*down_buffer;
  int 		num_up_buffer_free;
  int 		num_down_buffer_free;
  tick_t		fbdimm_cmd_bus_completion_time;/* Used by fb-dimm: time when the bus will be free */
  tick_t		data_bus_completion_time;/* Used by fb-dimm: time when the bus will be free */
  int	rank_members[2];



/**** implemented *********/


/**************************/


};


#endif
