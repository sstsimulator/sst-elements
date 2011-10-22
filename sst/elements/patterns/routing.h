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
#include <sst/core/cpunicEvent.h>
#include "machine_info.h"



class Router   {
    public:
	Router(class MachineInfo *machine) : _m(machine)
	{
	}

	~Router()   {
	}

	void attach_route(SST::CPUNicEvent *e, int dest_core);
	void show_route(std::string net_name, SST::CPUNicEvent *e, int src_rank, int dest_rank);


    private:
	void gen_route(SST::CPUNicEvent *e, int src, int dest, int width, int height,
		int depth, int x_wrap, int y_wrap, int z_wrap);

	MachineInfo *_m;


        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
	    ar & BOOST_SERIALIZATION_NVP(_m);
        }

} ;  // end of class Router


#endif  // _ROUTING_H_
