// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <string.h>
#include <sstream>

#include <sst/core/link.h>

#include "functionSM.h"
#include "ctrlMsg.h"

using namespace SST::Firefly;

const char* FunctionSM::m_functionName[] = {
    FOREACH_FUNCTION(GENERATE_STRING)
};

class DriverEvent : public SST::Event {
  public:
    DriverEvent( MP::Functor* _retFunc, int _retval ) :
        Event(),
        retFunc( _retFunc ),
        retval( _retval )
    { }
    MP::Functor* retFunc;
    int retval;
  private:

    NotSerializable(DriverEvent)
};

FunctionSM::FunctionSM( ComponentId_t id, SST::Params& params, ProtocolAPI* proto ) :
	SubComponent(id),
    m_sm( NULL ),
    m_params( params ),
    m_proto( proto )
{

    m_dbg.init("@t:FunctionSM::@p():@l ",
            params.find<uint32_t>("verboseLevel",0),
            0,
            Output::STDOUT );

    m_toDriverLink = configureSelfLink("ToDriver", "1 ps",
        new Event::Handler<FunctionSM>(this,&FunctionSM::handleToDriver));

    m_fromDriverLink = configureSelfLink("FromDriver", "1 ps",
        new Event::Handler<FunctionSM>(this,&FunctionSM::handleStartEvent));
    assert( m_fromDriverLink );

    m_toMeLink = configureSelfLink("ToMe", "1 ns",
        new Event::Handler<FunctionSM>(this,&FunctionSM::handleEnterEvent));
    assert( m_toMeLink );
}

FunctionSM::~FunctionSM()
{
    m_dbg.debug(CALL_INFO,1,0,"\n");
    for ( unsigned int i=0; i < m_smV.size(); i++ ) {
        delete m_smV[i];
    }
}

void FunctionSM::printStatus( Output& out )
{
    m_sm->printStatus(out);
}

void FunctionSM::setup( Info* info )
{
    char buffer[100];
    int nodeId =  info->worldRank();
    snprintf(buffer,100,"@t:%d:FunctionSM::@p():@l ", nodeId );
    m_dbg.setPrefix(buffer);

    m_smV.resize( NumFunctions );

    Params defaultParams;
    defaultParams.enableVerify(false);
    defaultParams.insert( "module" , m_params.find<std::string>("defaultModule","firefly"), true );
    defaultParams.insert( "enterLatency",
                        m_params.find<std::string>("defaultEnterLatency","0"), true );
    defaultParams.insert( "returnLatency",
                        m_params.find<std::string>("defaultReturnLatency","0"), true );
    defaultParams.insert( "smallCollectiveVN",
                        m_params.find<std::string>("smallCollectiveVN","0"), true );
    defaultParams.insert( "smallCollectiveSize",
                        m_params.find<std::string>("smallCollectiveSize","0"), true );
    defaultParams.insert( "verboseLevel", m_params.find<std::string>("verboseLevel","0"), true );
    std::ostringstream tmp;
    tmp <<  nodeId;
    defaultParams.insert( "nodeId", tmp.str(), true );

    for ( int i = 0; i < NumFunctions; i++ ) {
        std::string name = functionName( (FunctionEnum) i );
        Params tmp = m_params.get_scoped_params( name );
        defaultParams.insert( "name", name, true );
        initFunction( info, (FunctionEnum) i,
                                        name, defaultParams, tmp );
    }
}

void FunctionSM::initFunction( Info* info,
    FunctionEnum num, std::string name, Params& defaultParams, Params& params)
{
    std::string module = params.find<std::string>("module");
    if ( module.empty() ) {
        module = defaultParams.find<std::string>("module");
    }

    m_dbg.debug(CALL_INFO,3,0,"func=`%s` module=`%s`\n",
                            name.c_str(),module.c_str());

    if ( params.find<std::string>("name").empty() ) {
        params.insert( "name",  defaultParams.find<std::string>( "name" ), true );
    }

    if ( params.find<std::string>("verboseLevel").empty() ) {
        params.insert( "verboseLevel", defaultParams.find<std::string>( "verboseLevel" ), true );
    }

    if ( params.find<std::string>("enterLatency").empty() ) {
        params.insert( "enterLatency", defaultParams.find<std::string>( "enterLatency" ), true );
    }

    if ( params.find<std::string>("returnLatency").empty() ) {
        params.insert( "returnLatency", defaultParams.find<std::string>( "returnLatency" ), true );
    }

    if ( params.find<std::string>("smallCollectiveVN").empty() ) {
        params.insert( "smallCollectiveVN", defaultParams.find<std::string>( "smallCollectiveVN" ), true );
    }
    if ( params.find<std::string>("smallCollectiveSize").empty() ) {
        params.insert( "smallCollectiveSize", defaultParams.find<std::string>( "smallCollectiveSize" ), true );
    }

    params.insert( "nodeId", defaultParams.find<std::string>( "nodeId" ), true );

    m_smV[ num ] = (FunctionSMInterface*)loadModule( module + "." + name,
                             params );

    assert( m_smV[ Init ] );
    m_smV[ num ]->setInfo( info );

    if ( ! m_smV[ num ]->protocolName().empty() ) {
        m_smV[ num ]->setProtocol( m_proto );
    }
}

void FunctionSM::enter( )
{
    m_dbg.debug(CALL_INFO,3,0,"%s\n",m_sm->name().c_str());
    m_toMeLink->send( NULL );
}

void FunctionSM::start(int type, Callback callback,  SST::Event* e)
{
    assert( e );
    m_retFunc = NULL;
    m_callback = callback;
    assert( ! m_sm );
    m_sm = m_smV[ type ];
    m_dbg.debug(CALL_INFO,3,0,"%s enter\n",m_sm->name().c_str());
    m_fromDriverLink->send( m_sm->enterLatency(), e );
}

void FunctionSM::start( int type, MP::Functor* retFunc, SST::Event* e )
{
    assert( e );
    m_retFunc = retFunc;
    assert( ! m_sm );
    m_sm = m_smV[ type ];
    m_dbg.debug(CALL_INFO,3,0,"%s enter\n",m_sm->name().c_str());
    m_fromDriverLink->send( m_sm->enterLatency(), e );
}

void FunctionSM::handleStartEvent( SST::Event* e )
{
    Retval retval;
    assert( e );
    assert( m_sm );
    m_dbg.debug(CALL_INFO,3,0,"%s\n",m_sm->name().c_str());
    m_sm->handleStartEvent( e, retval );
    processRetval( retval );
}


void FunctionSM::handleEnterEvent( SST::Event* e )
{
    assert( m_sm );
    m_dbg.debug(CALL_INFO,3,0,"%s\n",m_sm->name().c_str());
    Retval retval;
    m_sm->handleEnterEvent( retval );
    processRetval( retval );
}

void FunctionSM::processRetval(  Retval& retval )
{
    if ( retval.isExit() ) {
        m_dbg.debug(CALL_INFO,3,0,"Exit %" PRIu64 "\n", retval.value() );
        if ( m_retFunc ) {
            DriverEvent* x = new DriverEvent( m_retFunc, retval.value() );
            m_toDriverLink->send( m_sm->returnLatency(), x );
        } else {
            m_callback();
        }
    } else if ( retval.isDelay() ) {
        m_dbg.debug(CALL_INFO,3,0,"Delay %" PRIu64 "\n", retval.value() );
        m_toMeLink->send( retval.value(), NULL );
    } else {
    }
}

void FunctionSM::handleToDriver( Event* e )
{
    assert( e );
    m_dbg.debug(CALL_INFO,3,0," returning\n");
    DriverEvent* event = static_cast<DriverEvent*>(e);
    if ( (*event->retFunc)( event->retval ) ) {
        delete event->retFunc;
    }
    m_sm = NULL;
    delete e;
}

