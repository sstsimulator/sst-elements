//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
//

#ifndef _MACHINE_INFO_H_
#define _MACHINE_INFO_H_

#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

// Storage for NIC parameters
typedef struct   {
    int index;
    int64_t inflectionpoint;
    int64_t latency;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
	ar & BOOST_SERIALIZATION_NVP(index);
	ar & BOOST_SERIALIZATION_NVP(inflectionpoint);
	ar & BOOST_SERIALIZATION_NVP(latency);
    }
} NICparams_t;



// Describe a far link
typedef struct   {
    // int src;   Implicit, it is my node
    int src_port;
    int dest;
    int dest_port;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
	ar & BOOST_SERIALIZATION_NVP(src_port);
	ar & BOOST_SERIALIZATION_NVP(dest);
	ar & BOOST_SERIALIZATION_NVP(dest_port);
    }
} FarLink_t;

struct _FLcompare   {
    bool operator () (const FarLink_t& x, const FarLink_t& y) const {return x.dest < y.dest;} 
};



class MachineInfo   {
    public:
	MachineInfo(SST::Params& params)   {
	    if (!init(params))   {
            SST::Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Machine info initialization failed!\n");
	    }
	}

	~MachineInfo()   {
	}


	int get_total_ranks(void)   {return total_ranks;}
	int get_total_cores(void)   {return total_cores;}

	int get_Net_width(void)   {return Net_width;}
	int get_Net_height(void)   {return Net_height;}
	int get_Net_depth(void)   {return Net_depth;}
	int get_NetXwrap(void)   {return NetXwrap;}
	int get_NetYwrap(void)   {return NetYwrap;}
	int get_NetZwrap(void)   {return NetZwrap;}

	int get_NoC_width(void)   {return NoC_width;}
	int get_NoC_height(void)   {return NoC_height;}
	int get_NoC_depth(void)   {return NoC_depth;}
	int get_NoCXwrap(void)   {return NoCXwrap;}
	int get_NoCYwrap(void)   {return NoCYwrap;}
	int get_NoCZwrap(void)   {return NoCZwrap;}

	int get_cores_per_NoC_router(void)   {return cores_per_NoC_router;}
	int get_cores_per_Net_router(void)   {return cores_per_Net_router;}
	int get_num_router_nodes(void)   {return num_router_nodes;}
	int get_cores_per_node(void)   {return cores_per_node;}

	int64_t get_LinkBandwidth(NIC_model_t nic)   {return LinkBandwidth[nic];}
	int64_t get_LinkLatency(NIC_model_t nic)   {return LinkLatency[nic];}
	int64_t get_IOLinkBandwidth(void)   {return IOLinkBandwidth;}
	int64_t get_IOLinkLatency(void)   {return IOLinkLatency;}

	int myNetX(void);
	int myNetY(void);
	int myNetZ(void);
	int myNoCX(void);
	int myNoCY(void);
	int myNoCZ(void);

	int myCore(void)   {return _my_core;}
	int myRank(void)   {return _my_rank;}
	int myNode(void);
	int myNetRouter(void);

	int destNode(int dest_core);
	int destNetRouter(int dest_node);

	SST::SimTime_t get_NICgap(NIC_model_t nic)   {return NICgap[nic];}

	bool FarLinkExists(int dest_node);
	int FarLinkExitPort(int dest_node);

	// Directly exposed vars
	int verbose;
	unsigned int debug;
	std::set<int> NICstat_ranks;
	std::list<NICparams_t> NICparams[NUM_NIC_MODELS];
	float NICsend_fraction[NUM_NIC_MODELS];



    private:
	bool init(SST::Params& params);
	void insert_inflection_point(NIC_model_t nic, int index, int64_t point);
	void insert_inflection_latency(NIC_model_t nic, int index, int64_t latency);
	bool error_check(void);
	void disp_banner(int disp_rank);

	// Input parameters for the machine architecture
	int _my_rank;
	int _my_core;
	int Net_width;
	int Net_height;
	int Net_depth;
	int NoC_width;
	int NoC_height;
	int NoC_depth;
	int cores_per_NoC_router;
	int cores_per_Net_router; // Calculated
	int num_router_nodes;

	// Are any dimensions wrapped?
	int NetXwrap;
	int NetYwrap;
	int NetZwrap;
	int NoCXwrap;
	int NoCYwrap;
	int NoCZwrap;

	// Calculated values
	int total_cores;
	int total_ranks;
	int cores_per_node;

	// Input parameters for the NIC models
	SST::SimTime_t NICgap[NUM_NIC_MODELS];
	std::set<FarLink_t, _FLcompare> FarLink;
	int FarLinkPortFieldWidth;
	int FarLinknum;
	int64_t LinkBandwidth[NUM_NIC_MODELS];
	int64_t LinkLatency[NUM_NIC_MODELS];
	int64_t IOLinkBandwidth;
	int64_t IOLinkLatency;


        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
	    ar & BOOST_SERIALIZATION_NVP(verbose);
	    ar & BOOST_SERIALIZATION_NVP(debug);
	    ar & BOOST_SERIALIZATION_NVP(NICstat_ranks);
	    ar & BOOST_SERIALIZATION_NVP(NICparams);
	    ar & BOOST_SERIALIZATION_NVP(NICsend_fraction);
	    ar & BOOST_SERIALIZATION_NVP(_my_rank);
	    ar & BOOST_SERIALIZATION_NVP(_my_core);
	    ar & BOOST_SERIALIZATION_NVP(Net_width);
	    ar & BOOST_SERIALIZATION_NVP(Net_height);
	    ar & BOOST_SERIALIZATION_NVP(Net_depth);
	    ar & BOOST_SERIALIZATION_NVP(NoC_width);
	    ar & BOOST_SERIALIZATION_NVP(NoC_height);
	    ar & BOOST_SERIALIZATION_NVP(NoC_depth);
	    ar & BOOST_SERIALIZATION_NVP(cores_per_NoC_router);
	    ar & BOOST_SERIALIZATION_NVP(cores_per_Net_router);
	    ar & BOOST_SERIALIZATION_NVP(num_router_nodes);
	    ar & BOOST_SERIALIZATION_NVP(NetXwrap);
	    ar & BOOST_SERIALIZATION_NVP(NetYwrap);
	    ar & BOOST_SERIALIZATION_NVP(NetZwrap);
	    ar & BOOST_SERIALIZATION_NVP(NoCXwrap);
	    ar & BOOST_SERIALIZATION_NVP(NoCYwrap);
	    ar & BOOST_SERIALIZATION_NVP(NoCZwrap);
	    ar & BOOST_SERIALIZATION_NVP(total_cores);
	    ar & BOOST_SERIALIZATION_NVP(total_ranks);
	    ar & BOOST_SERIALIZATION_NVP(cores_per_node);
	    ar & BOOST_SERIALIZATION_NVP(NICgap);
	    ar & BOOST_SERIALIZATION_NVP(FarLink);
	    ar & BOOST_SERIALIZATION_NVP(FarLinkPortFieldWidth);
	    ar & BOOST_SERIALIZATION_NVP(FarLinknum);
	    ar & BOOST_SERIALIZATION_NVP(LinkBandwidth);
	    ar & BOOST_SERIALIZATION_NVP(LinkLatency);
	    ar & BOOST_SERIALIZATION_NVP(IOLinkBandwidth);
	    ar & BOOST_SERIALIZATION_NVP(IOLinkLatency);
        }

} ;  // end of class MachineInfo

#endif  // _MACHINE_INFO_H_
