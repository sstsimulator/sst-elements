#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>

#include <inttypes.h>
#include <python/swig/pyobject.hh>
#include <sim/simulate.hh>
#include <util.h>
#include <debug.h>

#include "m5.h"

using namespace SST;

boost::mpi::communicator world;

static void enableDebug( std::string name );

M5::M5( ComponentId_t id, Params_t& params ) :
    Component( id),
    m_armed( false )
{
    // M5 variable
    want_info = false;
    
    if ( params.find( "debug" ) != params.end() ) {
        int level = strtol( params["debug"].c_str(), NULL, 0 );
       
        if ( level ) { 
            _dbg.level( level );
        }
    }
    if ( params.find( "M5debug" ) != params.end() ) {
        enableDebug( params[ "M5debug" ] );
    }

    if ( params.find( "info" ) != params.end() ) {
        if ( params["info"].compare("yes") == 0 ) {
            _info.enable();
        } 
    }

    std::ostringstream tmp;

    tmp << world.rank();

    _dbg.prepend(  tmp.str() + ":");
    _info.prepend(  tmp.str() + ":");

    std::string configFile = "";
    if ( params.find( "configFile" ) != params.end() ) {
      configFile = params["configFile"];
    }

    INFO( "configFile `%s`\n", configFile.c_str() );

    buildConfig( this, "m5", configFile );

    DBGX( 2, "call initAll\n" );
    initAll();

    DBGX( 2, "call regAllStats\n" );
    regAllStats();

    DBGX( 2, "call SimStartup\n" );
    SimStartup();
    DBGX( 2, "SimStartup done\n" );

    setClockFrequency(1000000000000);


    m_tc = registerTimeBase("1ps");
    if ( ! m_tc ) {
     //   _error( Port2Link, "registerTimeBase()\n");
    }
    m_self = configureSelfLink("self", m_tc,
            new SST::Event::Handler<M5>(this,&M5::selfEvent));
//    m_link->setDefaultTimeBase(m_tc);


    arm( 0 );

    if ( params.find( "registerExit" ) != params.end() ) {
        if( ! params["registerExit"].compare("yes") ) {
            INFO("registering exit\n");
            registerExit();
        }
    }

    DBGX( 2, "returning\n" );
}

M5::~M5()
{
    // do we really need to cleanup?
}

int M5::Setup()
{
    return 0;
}

void M5::catchup( SST::Cycle_t time ) 
{
    DBGX( 3, "SST-time=%lu, M5-time=%lu simulate(%lu)\n",
                                    time, curTick, time - curTick ); 

    simulate( time - curTick );

    m_event.cycles = m_event.time - time;
}

void M5::arm( SST::Cycle_t now )
{
    if ( ! m_armed && ! mainEventQueue.empty() )  {
        m_event.cycles = mainEventQueue.nextTick() - now;
        m_event.time = mainEventQueue.nextTick();

        DBGX( 3, "nextTick=%lu cycles=%lu\n", m_event.time, m_event.cycles );
        if ( now != 0 && m_event.cycles == 0 ) exit(1);

        DBGX( 3, "send %lu\n", now + m_event.cycles );
        m_self->Send( m_event.cycles, &m_event );

        m_armed = true;
    }
}

void M5::selfEvent( SST::Event* )
{
    m_armed = false;

    Cycle_t now = m_tc->convertToCoreTime( getCurrentSimTime(m_tc) );

    DBGX( 3, "currentTime=%lu cycles=%lu\n", m_event.time, m_event.cycles );

    SimLoopExitEvent* exitEvent = simulate( m_event.cycles );

    if ( ! exitEvent ) {
        arm( m_event.time );
    } else {
        unregisterExit();
        INFO( "exiting: time=%lu cause=`%s` code=%d\n", m_event.time,
                exitEvent->getCause().c_str(), exitEvent->getCode() );
    }
}

static void enableDebug( std::string name )
{

    if ( name.compare("all" ) ) return;
    Trace::enabled = true;
    for ( int i = 0; i < Trace::flags.size(); i++ ) {
//            printf("%s\n",Trace::flagStrings[i]);
        //    if ( ! strcmp( Trace::flagStrings[i], "Commit" ) ) 
            Trace::flags[i] = true;
    }
}
