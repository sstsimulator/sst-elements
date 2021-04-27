// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/stringize.h"
#include <sst/core/timeLord.h>


#include "emberengine.h"
#include "embergen.h"
#include "embermotiflog.h"
#include "libs/misc.h"

using namespace std;
using namespace SST::Ember;
using namespace SST::Hermes;

EmberEngine::EmberEngine(SST::ComponentId_t id, SST::Params& params) :
    Component( id ),
	currentMotif(0),
	m_motifDone(false),
	m_detailedCompute(NULL)
{
	// Get the level of verbosity the user is asking to print out, default is 1
	// which means don't print much.
	uint32_t verbosity = (uint32_t) params.find("verbose", 1);
	uint32_t mask = (uint32_t) params.find("verboseMask", 0);
	m_jobId = params.find("jobId", -1);


	std::ostringstream prefix;
	prefix << "@t:" << m_jobId << ":EmberEngine:@p:@l: ";

	output.init( prefix.str(), verbosity, mask, Output::STDOUT);

    std::ostringstream tmp;
    tmp << m_jobId;

    m_os  = loadUserSubComponent<OS>( "OS" );
    assert( m_os );

    m_nodePerf = m_os->getNodePerf();
    m_detailedCompute = m_os->getDetailedCompute();
    m_memHeapLink = m_os->getMemHeapLink();

    std::string motifLogFile = params.find<std::string>("motifLog", "");
    if("" != motifLogFile) {
        m_motifLogger = new EmberMotifLog(motifLogFile, m_jobId);
    } else {
        m_motifLogger = NULL;
    }
	output.verbose(CALL_INFO, 2, ENGINE_MASK, "\n");

	// create a map of all the available API's
	m_apiMap = createApiMap( m_os, this, params );
    assert( ! m_apiMap.empty() );

	motifParams.resize( params.find("motif_count", 1) );
	output.verbose(CALL_INFO, 2, ENGINE_MASK, "Identified %ld motifs "
                                    "to be simulated.\n", motifParams.size());

	for ( unsigned int i = 0;  i < motifParams.size(); i++ ) {
		std::ostringstream tmp;
    	tmp << i;

        //NetworkSim: Add the rankmapper parameter as motif parameters and pass it.
        params.insert("motif" + tmp.str() + ".rankmapper", params.find<string>("rankmapper", "ember.LinearMap"), true);

        //NetworkSim: Add the mapFile parameter as motif parameters and pass it.
        params.insert("motif" + tmp.str() + ".rankmap.mapFile", params.find<string>("mapFile", "mapFile.txt"), true);
        //NetworkSim->end

		motifParams[i] = params.get_scoped_params( "motif" + tmp.str() );
	}

    registerAsPrimaryComponent();

    // Init the first Motif
    m_generator = initMotif( motifParams[0], m_apiMap, m_jobId,
                        currentMotif, m_nodePerf );
    assert( m_generator );

	// Configure self link to handle event timing
	selfEventLink = configureSelfLink("self", "1ps",
		new Event::Handler<EmberEngine>(this, &EmberEngine::handleEvent));
    assert(selfEventLink);

	// Create a time converter for our compute events
	nanoTimeConverter =
        Simulation::getSimulation()->getTimeLord()->getTimeConverter("1ns");
}

EmberEngine::~EmberEngine() {
	ApiMap::iterator iter = m_apiMap.begin();
	for ( ; iter != m_apiMap.end(); ++ iter ) {
		delete iter->second;
	}

	if(NULL != m_motifLogger) {
		delete m_motifLogger;
	}
}

EmberEngine::ApiMap EmberEngine::createApiMap( OS* os,
                        SST::Component* owner, SST::Params params )
{
    ApiMap tmp;

    Params apiList = params.get_scoped_params( "api" );

    int apiNum = 0;
    while ( 1 ) {

	    std::ostringstream numStr;
   	    numStr << apiNum++;

        Params apiParams = apiList.get_scoped_params( numStr.str() );
        if ( apiParams.empty() ) {
            break;
        }

        std::string moduleName = apiParams.find<std::string>( "module" );
        assert( ! moduleName.empty() );

        Params modParams = params.get_scoped_params( moduleName );

        output.verbose(CALL_INFO, 2, ENGINE_MASK, "moduleName=%s\n", moduleName.c_str());

        Hermes::Interface* api = loadAnonymousSubComponent<Interface>( moduleName, "", 0, ComponentInfo::SHARE_NONE, modParams );
        assert( tmp.find( api->getName() ) == tmp.end() );

        output.verbose(CALL_INFO, 2, ENGINE_MASK, "api name=%s type=%s\n",api->getName().c_str(), api->getType().c_str() );
        api->setOS( os );

        EmberLib* lib = NULL;

        std::string type = api->getType();

        if ( type.length() > 0 ) {
            std::string emberLib = "ember." + type + "Lib";
            output.verbose(CALL_INFO, 2, ENGINE_MASK, "lib=%s\n",emberLib.c_str() );
            SST::Params x;
            lib = dynamic_cast<EmberLib*>( loadModule( emberLib, x ) );
            assert(lib);

            lib->initApi( api );
        } else {
            type = api->getName();
		}
        tmp[ type ] = new ApiInfo;
        tmp[ type ]->api = api;
        tmp[ type ]->lib = lib;
    }

    return tmp;
}

EmberGenerator* EmberEngine::initMotif( SST::Params params,
	const ApiMap& apiMap, int jobId, int motifNum, NodePerf* nodePerf )
{
    EmberGenerator* gen = NULL;

    // get the name of the motif
    std::string gentype = params.find<std::string>( "name" );
	assert( !gentype.empty() );

    // if(NULL != m_motifLogger) {
    //     m_motifLogger->logMotifStart(gentype, motifNum);
    // }

	output.verbose(CALL_INFO, 2, ENGINE_MASK, "motif=`%s`\n", gentype.c_str());

	if( gentype.empty()) {
		output.fatal(CALL_INFO, -1, "Error: You did not specify a generator"
                "or Ember to use\n");
	} else {
		params.insert("_jobId", SST::to_string( jobId ), true);
		params.insert("_motifNum", SST::to_string( motifNum ), true);
		assert( sizeof(this) == sizeof(uint64_t) );
		params.insert("_enginePtr", SST::to_string( reinterpret_cast<uint64_t>( this ) ), true);

		gen = loadAnonymousSubComponent<EmberGenerator>( gentype, "", 0, ComponentInfo::SHARE_NONE, params );
		gen->setup();

		if(NULL == gen) {
			output.fatal(CALL_INFO, -1, "Error: Could not load the "
                    "generator %s for Ember\n", gentype.c_str());
		}
	}

	// Make sure we don't stop the simulation until we are ready
    if ( gen->primary() ) {
        primaryComponentDoNotEndSim();
    }

    return gen;
}

void EmberEngine::init(unsigned int phase) {
	// Pass the init phases through to the OS layer
	m_os->_componentInit(phase);
}

void EmberEngine::finish() {
    ApiMap::iterator iter = m_apiMap.begin();
    for ( ; iter != m_apiMap.end(); ++ iter ) {
        iter->second->api->finish();
    }

	m_os->finish();
}

void EmberEngine::setup() {
	// Notify OS layer we are done with init phase
	// and are now in final bring up state

    m_os->_componentSetup();

    ApiMap::iterator iter = m_apiMap.begin();
    for ( ; iter != m_apiMap.end(); ++ iter ) {
        iter->second->api->setup();
    }

    std::ostringstream prefix;
    prefix << "@t:" << m_jobId << ":" << m_os->getRank() << ":EmberEngine:@p:@l: ";
    //std::cout << "@t:" << m_jobId << ":" << m_os->getRank() << ":EmberEngine:@p:@l: " << std::endl; //NetworkSim

    output.setPrefix( prefix.str() );

    if (NULL != m_motifLogger) {
        m_motifLogger->setRank(m_os->getRank());
    }

	// Prime the event queue
	issueNextEvent(0);
}

void EmberEngine::issueNextEvent(uint64_t nanoDelay) {

    output.debug(CALL_INFO, 8, ENGINE_MASK, "Engine issuing next event with delay %" PRIu64 "\n", nanoDelay);

    while ( evQueue.empty() ) {

        if ( ! m_motifDone ) {
            m_motifDone = refillQueue();
        }

        // if the event Queue is empty after a refill the motif is done
        if (  evQueue.empty() ) {
            if (NULL != m_motifLogger) {
                m_motifLogger->logMotifEnd(m_generator->getMotifName(),currentMotif);
            }
            // output.verbose(CALL_INFO, 1, MOTIF_START_STOP_MASK, "Motif finished: %s\n",m_generator->getMotifName().c_str());
            m_generator->completed( &output, getCurrentSimTimeNano() );
            if ( m_generator->primary() ) {
	            primaryComponentOKToEndSim();
            }
            delete m_generator;

            if ( ++currentMotif == motifParams.size() ) {
                return;
            } else {
                m_generator = initMotif( motifParams[currentMotif],
								m_apiMap, m_jobId, currentMotif, m_nodePerf );
                assert( m_generator );
                if (NULL != m_motifLogger) {
                    m_motifLogger->logMotifStart(currentMotif);
                }
                // output.verbose(CALL_INFO, 1, MOTIF_START_STOP_MASK, "Motif starting: %s\n",m_generator->getMotifName().c_str());

                m_motifDone = refillQueue();
            }
        }
    }

	EmberEvent* nextEv = evQueue.front();
	evQueue.pop();

	// issue the next event to the engine for deliver later
	selfEventLink->send(nanoDelay, nanoTimeConverter, nextEv);
}

bool EmberEngine::completeFunctor( int retval, EmberEvent* ev )
{
    output.debug(CALL_INFO, 2, ENGINE_MASK, "%s %s Event\n",
              ev->stateName( ev->state() ).c_str(), ev->getName().c_str());

    if ( ev->complete( getCurrentSimTimeNano(), retval ) ) {
        delete ev;
    }

	issueNextEvent(0);

    return true;
}

void EmberEngine::handleEvent(Event* ev) {

	// Cast out the event we are processing and then hand off to whatever
	// handlers we have created
	EmberEvent* eEv = static_cast<EmberEvent*>(ev);

    output.debug(CALL_INFO, 2, ENGINE_MASK, "%s %s Event\n",
              eEv->stateName( eEv->state() ).c_str(), eEv->getName().c_str());

    switch ( eEv->state() ) {
      case EmberEvent::Issue:

        eEv->issue( getCurrentSimTimeNano() );

	    selfEventLink->send( eEv->completeDelayNS() * 1000, ev );
        break;

      case EmberEvent::IssueFunctor:
        eEv->issue( getCurrentSimTimeNano(),
                new ArgStatic_Functor< EmberEngine, int, EmberEvent*, bool >(
                            this, &EmberEngine::completeFunctor, eEv ) );
        break;

      case EmberEvent::IssueCallback:
        eEv->issue( getCurrentSimTimeNano(),
                    std::bind( &EmberEngine::completeCallback, this, eEv, std::placeholders::_1 ) );
        break;

      case EmberEvent::IssueCallbackPtr:
		{
		    Callback* callback = new Callback;
		    *callback = std::bind( &EmberEngine::completeCallback, this, eEv, std::placeholders::_1 );
            eEv->issue( getCurrentSimTimeNano(), callback );
		}
        break;

      case EmberEvent::Complete:
        if ( eEv->complete( getCurrentSimTimeNano() ) ) {
            delete ev;
        }
	    issueNextEvent(0);
        break;
    }
}

EmberEngine::EmberEngine() :
    Component(-1)
{
    // for serialization only
}

