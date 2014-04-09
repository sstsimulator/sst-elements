//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
//

#ifndef _NIC_MODEL_H_
#define _NIC_MODEL_H_

#include <boost/serialization/list.hpp>
#include "patterns.h"
#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <cpunicEvent.h>
#include "machine_info.h"
#include "routing.h"
#include "NIC_stats.h"



class NIC_model   {
    public:
	NIC_model(MachineInfo *machine, NIC_model_t nic, SST::Link *self_link,
		void *obj, SST::SimTime_t (*rrr)(void *obj)) :
	    _m(machine),
	    _nic(nic),
	    _self_link(self_link),
	    NICtime_handler(rrr),
	    NICtime_obj(obj)

	{
	    bool do_print;


	    _my_rank= _m->myRank();

	    // If my stats were requested in the XML file, print them before exit
	    do_print= false;
	    if (_m->NICstat_ranks.find(_my_rank) != _m->NICstat_ranks.end())   {
		do_print= true;
	    }
	    nstats= new NIC_stats(_my_rank, type_name(_nic), do_print, _m);
	    rtr= new Router(_m);
	    send_link= NULL;
	    NextSendSlot= 0;
	    NextRecvSlot= 0;
	}

	~NIC_model()   {
	    delete nstats;
	    delete rtr;
	}



	SST::SimTime_t send(CPUNicEvent *e, int dest_rank);
	SST::SimTime_t delay(int bytes);
	void handle_rcv_events(SST::Event *sst_event);
	void set_send_link(SST::Link *link) {send_link= link;}

    private:

	int64_t get_NICparams(std::list<NICparams_t> params, int64_t msg_len,
		float send_fraction);

	MachineInfo *_m;
	NIC_model_t _nic;
	SST::Link *_self_link;
	SST::SimTime_t (*NICtime_handler)(void *obj);
	void *NICtime_obj;

	int _my_rank;
	SST::Link *send_link;
	NIC_stats *nstats;
	Router *rtr;

	// When can next message leave (or arrive)?
	SST::SimTime_t NextSendSlot;
	SST::SimTime_t NextRecvSlot;



        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
	    ar & BOOST_SERIALIZATION_NVP(_m);
	    ar & BOOST_SERIALIZATION_NVP(_nic);
	    ar & BOOST_SERIALIZATION_NVP(_self_link);
	    ar & BOOST_SERIALIZATION_NVP(NICtime_handler);
	    ar & BOOST_SERIALIZATION_NVP(NICtime_obj);

	    ar & BOOST_SERIALIZATION_NVP(_my_rank);
	    ar & BOOST_SERIALIZATION_NVP(send_link);
	    ar & BOOST_SERIALIZATION_NVP(nstats);
	    ar & BOOST_SERIALIZATION_NVP(rtr);
	    ar & BOOST_SERIALIZATION_NVP(NextSendSlot);
	    ar & BOOST_SERIALIZATION_NVP(NextRecvSlot);
        }

} ;  // end of class NIC_model


#endif  // _NIC_MODEL_H_
