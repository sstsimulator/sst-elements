/*
// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
*/

#ifndef _PATTERN_COMMON_H
#define _PATTERN_COMMON_H

#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include "patterns.h"
#include <sst/core/component.h>		// For SST::Params
#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <cpunicEvent.h>
#include "machine_info.h"
#include "NIC_model.h"



class Patterns   {
    public:
	Patterns()   {
	    _my_rank= -1;
	    _m= NULL;
	    my_self_link= NULL;
	    my_nvram_link= NULL;
	    my_storage_link= NULL;
	    nic[NoC]= NULL;
	    nic[Net]= NULL;
	    nic[Far]= NULL;
	    msg_seq= 0;
	}

	~Patterns()   {
	}

	void init(SST::Params& params, SST::Link *self_link,
		NIC_model *model[NUM_NIC_MODELS],
		SST::Link *nvram_link, SST::Link *storage_link,
		MachineInfo *m, int my_rank);

	void self_event_send(int event, int32_t tag= 0, SST::SimTime_t delay= 0);
	void event_send(int dest, int event, int32_t tag= 0, uint32_t msg_len= 0,
		const char *payload= NULL, int payload_len= 0, int blocking= -1);
	void storage_write(int data_size, int return_event);
	void nvram_write(int data_size, int return_event);
	SST::SimTime_t memdelay(int bytes);


    private:
	int _my_rank;
	MachineInfo *_m;

	SST::Link *my_self_link;
	SST::Link *my_nvram_link;
	SST::Link *my_storage_link;

	// Connection to our message passing models
	NIC_model *nic[NUM_NIC_MODELS];

	// Each message event gets a unique number for debugging
	uint64_t msg_seq;


        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
	    ar & BOOST_SERIALIZATION_NVP(_my_rank);
	    ar & BOOST_SERIALIZATION_NVP(_m);
	    ar & BOOST_SERIALIZATION_NVP(my_self_link);
	    ar & BOOST_SERIALIZATION_NVP(my_nvram_link);
	    ar & BOOST_SERIALIZATION_NVP(my_storage_link);
	    ar & BOOST_SERIALIZATION_NVP(nic);
	    ar & BOOST_SERIALIZATION_NVP(msg_seq);
        }

} ;  // end of class Patterns


#endif  /* _PATTERN_COMMON_H */
