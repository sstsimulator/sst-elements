// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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

	EmberEngine();			    		// For serialization
	EmberEngine(const EmberEngine&);    // Do not implement
	void operator=(const EmberEngine&); // Do not implement

};

}
}

#endif
