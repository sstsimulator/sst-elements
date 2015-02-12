//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _COMM_PATTERN_H
#define _COMM_PATTERN_H


#include "patterns.h"

#include <cpunicEvent.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "state_machine.h"
#include "machine_info.h"
#include "pattern_common.h"
#include "NIC_model.h"

using namespace SST;



#define DBG_COMM_PATTERN 1
#if DBG_COMM_PATTERN
#define _COMM_PATTERN_DBG(lvl, fmt, args...)\
    if (comm_pattern_debug >= lvl)   { \
	printf("%d:Comm_pattern::%s():%4d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _COMM_PATTERN_DBG(lvl, fmt, args...)
#endif



class Comm_pattern : public Component {
    public:
	// The constructor
        Comm_pattern(ComponentId_t id, Params& params) :
	    // constructor initializer list
            Component(id), params(params)
        {

	    NIC_model *NICmodel[NUM_NIC_MODELS];


      registerAsPrimaryComponent();
      primaryComponentDoNotEndSim();

	    // Figure out what the target machine looks like
	    machine= new MachineInfo(params);
	    my_rank= machine->myRank();
	    comm_pattern_debug= machine->verbose;


	    // Interface with SST

	    // Create a time converter
	    tc= registerTimeBase("1" TIME_BASE, true);


            // Create a channel to receive messages from our NICs
	    self_link= configureSelfLink("Me", new Event::Handler<Comm_pattern>
		    (this, &Comm_pattern::handle_self_events));
	    if (self_link == NULL)   {
		_ABORT(Comm_pattern, "That was no good!\n");
	    }


	    // Create an instance of a NIC model, then attach a handler
	    // to the SST link, and tell the NIC model about the link
	    // so it can send on it.
	    // This is a little convoluted because configureLink() is
	    // part of the SST Component object. I'd like to have this
	    // code in the NIC_model, but don't know how to do that cleanly.
	    // Do this for all three NIC models we have: Net, NoC, and Far.
	    for (int i= Net; i <= Far; i++)   {
		NIC_model *model;
		Link *link;
		const char *name= type_name((NIC_model_t)i);

		model= new NIC_model(machine, (NIC_model_t)i, self_link,
				(void *)this, Comm_pattern::wrapper_getComponentTime);

		link= configureLink(name, new Event::Handler<NIC_model>
			(model, &NIC_model::handle_rcv_events));
		if (link == NULL)   {
		    if (i != Far)   {
			// Net and NoC are mandatory; Far is optional
			_COMM_PATTERN_DBG(0, "The Comm pattern generator expects a link named \"%s\"\n",
			    name);
			_ABORT(Comm_pattern, "Check the input XML file! for %s\n", name);
		    }
		}
		model->set_send_link(link);
		NICmodel[i]= model;
	    }



            // Create a handler for events from local NVRAM
	    nvram= configureLink("NVRAM", new Event::Handler<Comm_pattern>
		    (this, &Comm_pattern::handle_nvram_events));
	    if (nvram == NULL)   {
		_COMM_PATTERN_DBG(0, "The Comm pattern generator expects a link named \"NVRAM\"\n");
		_ABORT(Comm_pattern, "Check the input XML file for NVRAM!\n");
	    }

            // Create a handler for events from the storage network
	    storage= configureLink("STORAGE", new Event::Handler<Comm_pattern>
		    (this, &Comm_pattern::handle_storage_events));
	    if (storage == NULL)   {
		_COMM_PATTERN_DBG(0, "The Comm pattern generator expects a link named \"STORAGE\"\n");
		_ABORT(Comm_pattern, "Check the input XML file! for STORAGE\n");
	    }

	    self_link->setDefaultTimeBase(tc);
	    nvram->setDefaultTimeBase(tc);
	    storage->setDefaultTimeBase(tc);

	    // Initialize the common functions we need
	    // FIXME: Since I gutted patterns() this should probably be called
	    // something other than common.
	    common= new Patterns();
	    common->init(params, self_link, NICmodel, nvram, storage, machine, my_rank);

	    num_ranks= machine->get_total_ranks();
	    SM= new State_machine(my_rank);

        }  // Done with initialization

        ~Comm_pattern()   {
	    delete common;
	    delete SM;
	    delete machine;
	}


	void self_event_send(int event_type, SimTime_t duration);
	void send_msg(int dest, int len, state_event sm_event);
	void send_msg(int dest, int len, state_event sm_event, int blocking);

	// Some patterns may be able to use these
	uint32_t next_power2(uint32_t v);
	bool is_pow2(int num);
	double SimTimeToD(SimTime_t t);
	void local_compute(int done_event, SimTime_t duration);
	void memcpy(int done_event, int bytes);
	void vector_op(int done_event, int doubles);

	MachineInfo *machine;
	int my_rank;
	int num_ranks;
	State_machine *SM;

	// The NIC model is not a component, but it needs to know what time it is.
	SST::SimTime_t getComponentTime(void)   {return getCurrentSimTime();}
	static SST::SimTime_t wrapper_getComponentTime(void *obj)
	{
	    Comm_pattern * mySelf= (Comm_pattern *)obj;
	    return mySelf->getComponentTime();
	}

    private:

#ifdef SERIALIZATION_WORKS_NOW
        Comm_pattern();  // For serialization only
#endif  // SERIALIZATION_WORKS_NOW
        Comm_pattern(const Comm_pattern &c);
	void handle_sst_events(CPUNicEvent *sst_event, const char *err_str);
	void handle_self_events(Event *sst_event);
	void handle_nvram_events(Event *sst_event);
	void handle_storage_events(Event *sst_event);


	// Input paramters for simulation
	Patterns *common;
	int comm_pattern_debug;


	// Interfacing with SST
        Params params;
	Link *self_link;
	Link *nvram;
	Link *storage;
	TimeConverter *tc;


        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(machine);
	    ar & BOOST_SERIALIZATION_NVP(my_rank);
	    ar & BOOST_SERIALIZATION_NVP(num_ranks);
	    ar & BOOST_SERIALIZATION_NVP(SM);
	    ar & BOOST_SERIALIZATION_NVP(common);
	    ar & BOOST_SERIALIZATION_NVP(comm_pattern_debug);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(self_link);
	    ar & BOOST_SERIALIZATION_NVP(nvram);
	    ar & BOOST_SERIALIZATION_NVP(storage);
	    ar & BOOST_SERIALIZATION_NVP(tc);
        }
};
#endif // _COMM_PATTERN_H
