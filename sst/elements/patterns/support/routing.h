//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
//

#ifndef _ROUTING_H_
#define _ROUTING_H_

#include "patterns.h"
#include <sst/core/sst_types.h>
#include <cpunicEvent.h>
#include "machine_info.h"



class Router   {
    public:
	Router(class MachineInfo *machine) : _m(machine)
	{
	}

	~Router()   {
	}

	void attach_route(CPUNicEvent *e, int dest_core);
	void show_route(std::string net_name, CPUNicEvent *e, int src_rank, int dest_rank);


    private:
	void gen_route(CPUNicEvent *e, int src, int dest, int width, int height,
		int depth, int x_wrap, int y_wrap, int z_wrap);

	MachineInfo *_m;

} ;  // end of class Router


#endif  // _ROUTING_H_
