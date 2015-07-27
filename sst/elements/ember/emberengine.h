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

private:
	bool refillQueue() {
		return m_generator->generate( evQueue );
	}
	void handleEvent(SST::Event* ev);
	void issueNextEvent(uint64_t nanoSecDelay);
    bool completeFunctor( int retval, EmberEvent* ev ); 

	Hermes::OS*	m_os;

    struct ApiInfo {
        Hermes::Interface* api;
        EmberGeneratorData* data;
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

	EmberEngine();			    		// For serialization
	EmberEngine(const EmberEngine&);    // Do not implement
	void operator=(const EmberEngine&); // Do not implement

	////////////////////////////////////////////////////////
    friend class boost::serialization::access;
    template<class Archive>
	void save(Archive & ar, const unsigned int version) const
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    }

    template<class Archive>
	void load(Archive & ar, const unsigned int version)
    {
    	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    }

	BOOST_SERIALIZATION_SPLIT_MEMBER()
};

}
}

#endif
