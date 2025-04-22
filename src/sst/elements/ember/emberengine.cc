// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include <sst/core/timeLord.h>

#include "emberengine.h"
#include "embergen.h"
#include "embermotiflog.h"
#include "libs/misc.h"

using namespace std;
using namespace SST::Ember;
using namespace SST::Hermes;

EmberEngine::EmberEngine( SST::ComponentId_t id, SST::Params& params ) :
    Component( id ),
    m_currentMotif( 0 ),
    m_motifDone( false ),
    m_detailedCompute( nullptr )
{
    // Get the level of verbosity the user is asking to print out, default is 1
    // which means don't print much.
    uint32_t verbosity = (uint32_t) params.find( "verbose", 1 );
    uint32_t mask = (uint32_t) params.find( "verboseMask", 0 );
    m_jobId = params.find( "jobId", -1 );
    bool statsPerMotif = (bool) params.find( "enableMpiStatsPerMotif", 1 );

    std::ostringstream prefix;
    prefix << "@t:" << m_jobId << ":EmberEngine:@p:@l: ";

    output.init( prefix.str(), verbosity, mask, Output::STDOUT );

    std::ostringstream tmp;
    tmp << m_jobId;

    m_os  = loadUserSubComponent<OS>( "OS" );
    assert( m_os );

    m_nodePerf = m_os->getNodePerf();
    m_detailedCompute = m_os->getDetailedCompute();
    m_memHeapLink = m_os->getMemHeapLink();

    std::string motifLogFile = params.find<std::string>( "motifLog", "" );
    if ( "" != motifLogFile ) {
        m_motifLogger = loadComponentExtension<EmberMotifLog>( motifLogFile, m_jobId );
    } else {
        m_motifLogger = nullptr;
    }
    output.verbose( CALL_INFO, 2, ENGINE_MASK, "\n" );

    // create a map of all the available API's
    m_apiMap = createApiMap( m_os, this, params );
    assert( !m_apiMap.empty() );

    m_numMotifs = params.find( "motif_count", 1 );
    output.verbose( CALL_INFO, 2, ENGINE_MASK, "Identified %u motifs "
                    "to be simulated.\n", m_numMotifs );

    if ( m_numMotifs == 0 ) {
        output.fatal( CALL_INFO, -1, "Error: need to specify at least one motif\n" );
    }

    for ( int i = 0;  i < m_numMotifs; i++ ) {
        std::ostringstream tmp;
        tmp << i;
        //NetworkSim: Add the rankmapper parameter as motif parameters and pass it.
        params.insert( "motif" + tmp.str() + ".rankmapper",
                       params.find<string>( "rankmapper", "ember.LinearMap" ), true );
        //NetworkSim: Add the mapFile parameter as motif parameters and pass it.
        params.insert( "motif" + tmp.str() + ".rankmap.mapFile",
                       params.find<string>("mapFile", "mapFile.txt"), true );

        SST::Params motifParams = params.get_scoped_params( "motif" + tmp.str() );

        EmberGenerator* gen = createMotif( motifParams,
                                           m_apiMap,
                                           m_jobId,
                                           i,
                                           m_nodePerf,
                                           statsPerMotif );
        assert( gen );
        m_motifs.push_back( gen );
    }

    registerAsPrimaryComponent();

    // Init the first Motif
    assert(m_currentMotif == 0);
    m_generator = m_motifs[0];
    assert( m_generator );
    m_generator->configure();

    output.verbose( CALL_INFO, 4, MOTIF_START_STOP_MASK,
                    "Starting motif: %s\n", m_generator->getMotifName().c_str() );

    // Make sure we don't stop the simulation until we are ready
    if ( m_generator->primary() ) {
        primaryComponentDoNotEndSim();
    }

    // Configure self link to handle event timing
    auto eventHandler = new Event::Handler<EmberEngine>( this, &EmberEngine::handleEvent );
    selfEventLink = configureSelfLink( "self", "1ps", eventHandler );
    assert( selfEventLink );

    // Create a time converter for our compute events
    nanoTimeConverter = getTimeConverter( "1ns" );
}

EmberEngine::~EmberEngine() {
    ApiMap::iterator iter = m_apiMap.begin();
    for ( ; iter != m_apiMap.end(); ++ iter ) {
        delete iter->second;
    }

    if (nullptr != m_motifLogger) {
        delete m_motifLogger;
    }

    for ( size_t i = 0; i < m_motifs.size(); i++ ) {
        if (m_motifs[i]) {
            std::cout << "WARNING: Motif " << m_motifs[i]->getMotifName() << " has not completed\n";
            delete m_motifs[i];
        }
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
            lib = loadModule<EmberLib>( emberLib, x );
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

EmberGenerator* EmberEngine::createMotif( SST::Params params,
	const ApiMap& apiMap, int jobId, int motifNum, NodePerf* nodePerf, bool statsPerMotif )
{
    EmberGenerator* gen = NULL;

    // get the name of the motif
    std::string gentype = params.find<std::string>( "name" );

    // if(NULL != m_motifLogger) {
    //     m_motifLogger->logMotifStart(gentype, motifNum);
    // }

    if( gentype.empty() ) {
        output.fatal( CALL_INFO, -1, "Error: You did not specify a generator "
                                     "or Ember to use\n" );
    } else {
	    output.verbose( CALL_INFO, 2, ENGINE_MASK, "motif=`%s`\n", gentype.c_str() );

        params.insert( "_jobId", std::to_string( jobId ), true );
        params.insert( "_motifNum", std::to_string( motifNum ), true );
        assert( sizeof( this ) == sizeof( uint64_t ) );
        params.insert( "_enginePtr", std::to_string( reinterpret_cast<uint64_t>( this ) ), true);
        params.insert( "mpiStatsPerMotif", std::to_string( statsPerMotif ), true );

        gen = loadAnonymousSubComponent<EmberGenerator>( gentype, "", 0, ComponentInfo::INSERT_STATS, params );
        if ( !gen ) {
            output.fatal( CALL_INFO, -1, "Error: Could not load the "
                                         "generator %s for Ember\n", gentype.c_str() );
        }
        gen->setup();
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

    // This loop tries to generate at least one event.
    // When the currently executed motif gets drained, continue with the next motif,
    // until there are no more motifs to simulate.
    while ( evQueue.empty() ) {

        if ( !m_motifDone ) {
            // Enque some new events
            m_motifDone = refillQueue();
        }

        if ( evQueue.empty() ) {

            // No more events in the current motif

            // Finalize the current motif
            if ( NULL != m_motifLogger ) {
                m_motifLogger->logMotifEnd( m_generator->getMotifName(), m_currentMotif );
            }

            output.verbose( CALL_INFO, 4, MOTIF_START_STOP_MASK,
                            "Motif finished: %s\n", m_generator->getMotifName().c_str() );

            m_generator->completed( &output, getCurrentSimTimeNano() );

            const bool wasPrimary = m_generator->primary();

            delete m_generator;
            m_motifs[m_currentMotif] = nullptr;

            m_currentMotif++;

            // See if there are more motifs to simulate

            if ( m_currentMotif == m_numMotifs ) {
                output.verbose( CALL_INFO, 4, MOTIF_START_STOP_MASK,
                            "All (%d) motifs have finished\n", m_numMotifs );
                if ( wasPrimary ) {
                    primaryComponentOKToEndSim();
                }

                return;
            }

            m_generator = m_motifs[m_currentMotif];
            assert( m_generator );
            m_generator->configure();

            const bool isPrimary = m_generator->primary();
            if ( wasPrimary != isPrimary ) {
                if ( isPrimary ) {
                    primaryComponentDoNotEndSim();
                }
                else {
                    primaryComponentOKToEndSim();
                }
            }

            if ( NULL != m_motifLogger ) {
                m_motifLogger->logMotifStart( m_currentMotif );
            }

            output.verbose( CALL_INFO, 4, MOTIF_START_STOP_MASK,
                            "Motif starting: %s\n", m_generator->getMotifName().c_str());

            m_motifDone = refillQueue();
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

