// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization.h"
#include <sst/core/timeLord.h>
#include "emberengine.h"
#include "embergen.h"


using namespace std;
using namespace SST::Ember;
using namespace SST::Hermes;

EmberEngine::EmberEngine(SST::ComponentId_t id, SST::Params& params) :
    Component( id ),
	currentMotif(0),
	m_motifDone(false)
{
	// Get the level of verbosity the user is asking to print out, default is 1
	// which means don't print much.
	uint32_t verbosity = (uint32_t) params.find_integer("verbose", 1);
	m_jobId = params.find_integer("jobId", -1);

	std::ostringstream prefix;
	prefix << "@t:" << m_jobId << ":EmberEngine:@p:@l: ";

	output.init( prefix.str(), verbosity, (uint32_t) 0, Output::STDOUT);

    Params osParams = params.find_prefix_params("os.");

    std::string osName = osParams.find_string("name");
    assert( ! osName.empty() );
    std::string osModuleName = osParams.find_string("module");
    assert( ! osModuleName.empty() );

    Params modParams = params.find_prefix_params( osName + "." );

    m_os  = dynamic_cast<OS*>( loadModuleWithComponent(
                            osModuleName, this, modParams ) );
    assert( m_os );

	// create a map of all the available API's
	m_apiMap = createApiMap( m_os, this, params );
    assert( ! m_apiMap.empty() );

	motifParams.resize( params.find_integer("motif_count", 1) );
	output.verbose(CALL_INFO, 2, 0, "Identified %" PRIu64 " motifs "
                                    "to be simulated.\n", motifParams.size());
	
	for ( unsigned int i = 0;  i < motifParams.size(); i++ ) {
		std::ostringstream tmp;
    	tmp << i;
		motifParams[i] = params.find_prefix_params( "motif" + tmp.str() + "." );
	} 

    // Init the first Motif
    m_generator = initMotif( motifParams[0], m_apiMap, m_jobId, currentMotif );
    assert( m_generator );
    
	// Configure self link to handle event timing
	selfEventLink = configureSelfLink("self", "1ps",
		new Event::Handler<EmberEngine>(this, &EmberEngine::handleEvent));
    assert(selfEventLink);

	// Make sure we don't stop the simulation until we are ready
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

	// Create a time converter for our compute events
	nanoTimeConverter =
        Simulation::getSimulation()->getTimeLord()->getTimeConverter("1ns");
}

EmberEngine::~EmberEngine() {
}

EmberEngine::ApiMap EmberEngine::createApiMap( OS* os, 
                        SST::Component* owner, SST::Params params )
{
    ApiMap tmp;

    Params apiList = params.find_prefix_params( "api." );
    
    int apiNum = 0;
    while ( 1 ) {

	    std::ostringstream numStr;
   	    numStr << apiNum++;

        Params apiParams = apiList.find_prefix_params( numStr.str() + "." );
        if ( apiParams.empty() ) {
            break;
        }

        std::string moduleName = apiParams.find_string( "module" );
        assert( ! moduleName.empty() );
        Params modParams = apiParams.find_prefix_params( "params" );

        Hermes::Interface* api = dynamic_cast<Interface*>(
                        owner->loadModuleWithComponent( 
                            moduleName, owner, modParams ) );
        assert( tmp.find( api->getName() ) == tmp.end() );

        api->setOS( os );

        tmp[ api->getName() ] = new ApiInfo;
        tmp[ api->getName() ]->data = NULL;
        tmp[ api->getName() ]->api = api;
    }

    return tmp;
}

EmberGenerator* EmberEngine::initMotif( SST::Params params,
							const ApiMap& apiMap, int jobId, int motifNum ) 
{
    EmberGenerator* gen = NULL;

    // get the name of the motif
    std::string gentype = params.find_string( "name" );
	assert( !gentype.empty() );

    // get the api the motif uses
    std::string api = params.find_string("api" );

	output.verbose(CALL_INFO, 2, 0, "api=`%s` motif=`%s`\n", 
										api.c_str(), gentype.c_str());

	if( gentype.empty()) {
		output.fatal(CALL_INFO, -1, "Error: You did not specify a generator" 
                "or Ember to use\n");
	} else {
		gen = dynamic_cast<EmberGenerator*>(
                loadModuleWithComponent(gentype, this, params ) );

		if(NULL == gen) {
			output.fatal(CALL_INFO, -1, "Error: Could not load the "
                    "generator %s for Ember\n", gentype.c_str());
		}
	}

    if ( ! api.empty() ) {
        ApiInfo* info = apiMap.find(api)->second;
        gen->initOutput( &output );

        gen->initAPI( info->api );
        gen->initData( &info->data );
  	    info->data->jobId = jobId;
   	    info->data->motifNum = motifNum;
    }

    return gen;
}

void EmberEngine::init(unsigned int phase) {
	// Pass the init phases through to the OS layer
	m_os->_componentInit(phase);	
}

void EmberEngine::finish() {
}

void EmberEngine::setup() {
	// Notify OS layer we are done with init phase
	// and are now in final bring up state
	
    m_os->_componentSetup();	

    std::ostringstream prefix;
    prefix << "@t:" << m_jobId << ":" << m_os->getNid() << ":EmberEngine:@p:@l: ";

    output.setPrefix( prefix.str() );

	// Prime the event queue 
	issueNextEvent(0);
}

void EmberEngine::issueNextEvent(uint64_t nanoDelay) {

    output.verbose(CALL_INFO, 8, 0, "Engine issuing next event with delay %" PRIu64 "\n", nanoDelay);

    while ( evQueue.empty() ) {

        if ( ! m_motifDone ) {
            m_motifDone = refillQueue();
        }

        // if the event Queue is empty after a refill the motif is done
        if (  evQueue.empty() ) {
            m_generator->finish( &output, getCurrentSimTimeNano() );
            delete m_generator;

            if ( ++currentMotif == motifParams.size() ) {
	            primaryComponentOKToEndSim();
                return;
            } else {
                m_generator = initMotif( motifParams[currentMotif],
											m_apiMap, m_jobId, currentMotif );
                assert( m_generator );

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
    output.verbose(CALL_INFO, 1, 0, "%s %s Event\n", 
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

    output.verbose(CALL_INFO, 1, 0, "%s %s Event\n", 
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

BOOST_CLASS_EXPORT(EmberEngine)
