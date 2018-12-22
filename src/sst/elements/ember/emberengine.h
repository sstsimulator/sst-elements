// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_ENGINE
#define _H_EMBER_ENGINE

#include <queue>

#include <sst/core/elementinfo.h>
#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/elements/hermes/hermes.h>

#include "embermotiflog.h"
#include "embergen.h"

namespace SST {
namespace Ember {

class EmberEvent;
class EmberGeneratorData;

class EmberEngine : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(
        EmberEngine,
        "ember",
        "EmberEngine",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Base communicator motif engine.",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "module", "Sets the OS module", ""},
        { "verbose", "Sets the output verbosity of the component", "0" },
        { "jobId", "Sets the job id", "-1"},
        { "netMapName", "used internally", "-1"},
        { "_apiName", "used internally", "-1"},
        { "_jobId", "used internally", "-1"},
        { "_motifNum", "used internally", "-1"},

        { "motif_count", "Sets the number of motifs which will be run in this simulation, default is 1", "1"},

        { "distribModule", "Sets the distribution SST module for compute modeling, default is a constant distribution of mean 1", "1.0"},

        { "rankmapper", "Sets the rank mapping SST module to load to rank translations, default is linear mapping", "ember.LinearMap" },

        { "mapFile", "Sets the name of the input file for custom map", "mapFile.txt" },

        { "motif%(motif_count)d", "Sets the event generator or motif for the engine", "ember.EmberPingPongGenerator" },

        { "name", "Sets the event generator or motif for the engine", "ember.EmberPingPongGenerator" },
        { "api", "Sets the api used by a motif", "hermesParams" },

        { "spyplotmode", "Sets the spyplot generation mode, 0 = none, 1 = spy on sends", "0" },

        { "motifLog", "Sets a file path to a file where motif execution details are written, empty = no log", "" },

        { "Send_bin_width", "Bin width of the send time histogram", "5" },
        { "Compute_bin_width", "Bin width of the compute time histogram", "5" },
        { "Init_bin_width", "Bin width of the init time histogram", "5" },
        { "Finalize_bin_width", "Bin width of the finalize time histogram", "5"},
        { "Recv_bin_width", "Bin width of the recv time histogram", "5" },
        { "Rank_bin_width", "Bin width of the rank time histogram", "5" },
        { "Size_bin_width", "Bin width of the size time histogram", "5" },
        { "Recv_bin_width", "Bin width of the recv time histogram", "5" },
        { "Irecv_bin_width", "Bin width of the irecv time histogram", "5" },
        { "Isend_bin_width", "Bin width of the isend time histogram", "5" },
        { "Wait_bin_width", "Bin width of the wait time histogram", "5" },
        { "Waitall_bin_width", "Bin width of the waitall time histogram", "5" },
        { "Waitany_bin_width", "Bin width of the waitany time histogram", "5" },
        { "Barrier_bin_width", "Bin width of the barrier time histogram", "5" },
        { "Recvsize_bin_width", "Bin width of the recv sizes (bytes) histogram", "64" },
        { "Sendsize_bin_width", "Bin width of the send sizes (bytes) histogram", "64" },
        { "Allreduce_bin_width", "Bin width of the allreduce time histogram", "5" },
        { "Alltoall_bin_width", "Bin width of the alltoall time histogram", "5" },
        { "Alltoallv_bin_width", "Bin width of the alltoallv time histogram", "5" },
        { "Reduce_bin_width", "Bin width of the reduce time histogram", "5" },
        { "Bcast_bin_width", "Bin width of the bcast time histogram", "5" },
        { "Commsplit_bin_width", "Bin width of the comm_split time histogram", "5" },
        { "Commcreate_bin_width", "Bin width of the comm_create time histogram", "5" },
        { "Commdestroy_bin_width", "Bin width of the comm_destroy time histogram", "5" },
        { "Gettime_bin_width", "Bin width of the gettime time histogram", "5" },


        { "noisegen", "Sets the noise generator for the system", "constant" },
        { "noisemean", "Sets the mean of a Gaussian noise generator", "1.0" },
        { "noisestddev", "Sets the standard deviation of a noise generator", "0.1" },
    )

    SST_ELI_DOCUMENT_PORTS(
        {"detailed%(num_vNics)d", "Port connected to the detailed model", {}},
        {"nic", "Port connected to the nic", {}},
        {"loop", "Port connected to the loopBack", {}},
        {"memoryHeap", "Port connected to the memory heap", {}},
    )

public:
	EmberEngine( SST::ComponentId_t id, SST::Params& params );
	~EmberEngine();
	void setup();
	void finish();
	void init( unsigned int phase );

	Output* getOutput() { return &output; }
	Hermes::Interface* getAPI(std::string name) { 
        if ( m_apiMap.find(name) == m_apiMap.end() ) {
            return NULL;
        }
        return m_apiMap[name]->api; 
    }
	Hermes::NodePerf* getNodePerf( ) { return m_nodePerf; } 
	Thornhill::DetailedCompute* getDetailedCompute() {
		return m_detailedCompute;
	}


	FamAddrMapper* getFamAddrMapper() {
		return m_famAddrMapper;
	}

	Thornhill::MemoryHeapLink* getMemHeapLink() {
		return m_memHeapLink;
	}

    EmberLib* getLib( std::string name ) {
        assert( m_apiMap.find( name ) != m_apiMap.end() );
        return m_apiMap[name]->lib; 
    }

private:
	bool refillQueue() {
		return m_generator->generate( evQueue );
	}

    std::string getComputeModelName() {
       if ( m_detailedCompute ) {
           return m_detailedCompute->getModelName();
       }
       return "";
    }

	void handleEvent(SST::Event* ev);
	void issueNextEvent(uint64_t nanoSecDelay);

    void completeCallback( EmberEvent* ev, int retval ) {
        completeFunctor(retval, ev);
    }
    bool completeFunctor( int retval, EmberEvent* ev ); 

	Hermes::OS*	m_os;

    struct ApiInfo {
        Hermes::Interface* api;
        EmberGeneratorData* data;
        EmberLib*           lib;
    };

    typedef std::map< std::string, ApiInfo* > ApiMap; 

    ApiMap createApiMap( Hermes::OS* os, SST::Component*, SST::Params );
    EmberGenerator* initMotif( SST::Params, const ApiMap&,
					int jobId, int motifNum, Hermes::NodePerf* nodePerf );

	int         m_jobId;
	uint32_t    currentMotif;
    bool        m_motifDone;
    ApiMap      m_apiMap;
	Output      output;

	std::queue<EmberEvent*> evQueue;

    Hermes::NodePerf*   m_nodePerf;
	EmberGenerator*     m_generator;
	SST::Link*          selfEventLink;
	SST::TimeConverter* nanoTimeConverter;
	EmberMotifLog*      m_motifLogger;

	std::vector<SST::Params> motifParams;
	Thornhill::DetailedCompute* m_detailedCompute;
	Thornhill::MemoryHeapLink*  m_memHeapLink;
	FamAddrMapper* m_famAddrMapper;

	EmberEngine();			    		// For serialization
	EmberEngine(const EmberEngine&);    // Do not implement
	void operator=(const EmberEngine&); // Do not implement

};

}
}

#endif
