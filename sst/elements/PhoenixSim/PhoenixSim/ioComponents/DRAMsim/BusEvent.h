#ifndef BUSEVENT__H
#define BUSEVENT__H


#include "DRAMaddress.h"

class BusEvent {

public:

	BusEvent();


    bool 	already_run;
	int     attributes;
	uint64_t     address;        /* assume to be < 32 bits for now */
	int 	trace_id;
	tick_t  timestamp;      /* time stamp will now have a large dynamic range, but only 53 bit precision */
	DRAMaddress addr; // Added only to support sending addresses of banks/ranks/chans/rows pre-decided
	uint8_t length; // Used to set the burst length of  request





};
#endif


