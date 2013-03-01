#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/linkMap.h>

#include <dll/gem5dll.hh>
#include <sim/simulate.hh>
#include "portLink.h"
#include "util.h"
#include "debug.h"

#include "m5.h"

#define curTick libgem5::CurTick

using namespace SST;
using namespace SST::M5;

SST::M5::M5::M5( ComponentId_t id, Params_t& params ) :
    IntrospectedComponent( id ),
    m_numRegisterExits( 0 ),
    m_barrier( NULL ),
    params( params ),  //for power
    m_m5ticksPerSSTclock( 1000000000 )
{
    libgem5::SetWantInfo( false );
    libgem5::SetRemoteGDBPort(0);

    // a flag for telling if fastforwarding is used 
    FastForwarding_flag=false;

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

    std::string configFile = "";
    if ( params.find( "configFile" ) != params.end() ) {
      configFile = params["configFile"];
    }

    m_statFile = params.find_string("statFile");
	m_init_link_name = params.find_string("mem_initializer_port");

    // It makes things easier if we match the m5 simulation clock with 
    // the SST simulation clock. We don't have access to the SST clock but
    // it is 1 ps.
    unsigned long m5_freq = 1000000000000; 
    libgem5::SetClockFrequency( m5_freq );

    INFO( "configFile `%s`\n", configFile.c_str() );
    m_objectMap = buildConfig( this, "m5", configFile, params );

    TimeConverter *minPartTC = Simulation::getSimulation()->getMinPartTC();
    if ( minPartTC ) {
        m_m5ticksPerSSTclock = minPartTC->getFactor();
    }
    m_fooTicks = m_m5ticksPerSSTclock;

    float clockFreqKhz = (m5_freq / m_m5ticksPerSSTclock) / 1000; 

    std::string strBuf;
    strBuf.resize(32);
    snprintf( &strBuf[0], 32, "%.0f khz", clockFreqKhz );

    DBGX( 3, "SST sync factor %i, `%s`\n", 
                        m_m5ticksPerSSTclock, strBuf.c_str() ); 

    registerClock( strBuf, new SST::Clock::Handler< M5 >( this, &M5::clock ) );

    if ( params.find( "registerExit" ) != params.end() ) {
        if( ! params["registerExit"].compare("yes") ) {
            INFO("registering exit\n");
            IntrospectedComponent::registerExit();
        }
    }

    int numBarrier = params.find_integer( "numBarrier" );
    if ( numBarrier > 0 ) {
        m_barrier = new BarrierAction( numBarrier );
    }

    //
    // for power and thermal modeling
    //
    #ifdef M5_WITH_POWER
    Init_Power();
    #endif
}

SST::M5::M5::~M5()
{
}


void SST::M5::M5::init(unsigned int phase)
{
	if ( !phase ) {
		DBGX( 2, "call initAllObjects()\n" );
		libgem5::InitAllObjects();
		DBGX( 2, "initAllObjects\n" );

		/* Because there's no clean way to get my link (that was configured elsewhere... */
		if ( m_init_link_name != "" ) {
			SST::Link* memInitLink = Simulation::getSimulation()->getComponentLinkMap(getId())->getLink(m_init_link_name);
			assert( memInitLink );
			/* TODO: Should be part of intitialization
			 * Also, the call to InitAllObjects will need to be done
			 * before this call. */
			std::vector<libgem5::Blob> blobs;
			libgem5::getInitializedMemory(blobs);

			/* blobs -> memInitLink */
			for ( std::vector<libgem5::Blob>::iterator i = blobs.begin() ; i != blobs.end() ; ++i ) {
				SST::Interfaces::MemEvent *ev = new SST::Interfaces::MemEvent(this, i->address, SST::Interfaces::WriteReq);
				ev->setPayload(i->size, i->data);

				memInitLink->sendInitData(ev);

				/* Save memory */
				delete i->data;
				i->data = NULL;
				i->size = 0;
			}
		}

	}

	/* Setup Links */
	for ( objectMap_t::iterator i = m_objectMap.begin() ; i != m_objectMap.end() ; ++i ) {
		Gem5Object_t *obj = i->second;
		for ( std::deque<void*>::iterator j = obj->links.begin() ; j != obj->links.end() ; ++j ) {
			PortLink *link = static_cast<PortLink*>(*j);
			link->setup();
		}
	}
}

int SST::M5::M5::Setup()
{

    #ifdef M5_WITH_POWER
    Setup_Power();
    #endif

    return 0;
}

void SST::M5::M5::registerExit(void)
{
    DBGX(2,"m_numRegisterExits %d\n",m_numRegisterExits);
    if ( m_numRegisterExits == 0) {
        IntrospectedComponent::registerExit();
    } 
    ++m_numRegisterExits;
}

void SST::M5::M5::exit( int status )
{
    DBGX(2,"M5::exit() status=%d\n",status);
    --m_numRegisterExits;
    if ( m_numRegisterExits == 0) {
        libgem5::SchedExitEvent();
    } 
}

bool SST::M5::M5::catchup()
{
    Cycle_t cycles = SST::Simulation::getSimulation()-> getCurrentSimCycle();
    Cycle_t ticks = cycles - curTick();

    DBGX(5,"m5 curTick()=%lu sst getCurrentSimCycle()=%lu\n", 
						curTick(), cycles );

    m_fooTicks -= ticks;

    ::SimLoopExitEvent* exitEvent = simulate( ticks );
    bool rc = exitEvent->getCode() != 256;
    delete exitEvent;
    return rc;  
}

bool SST::M5::M5::clock( SST::Cycle_t cycle )
{
    DBGX( 5, "current_cycle=%lu\n", cycle * m_m5ticksPerSSTclock );
    DBGX( 5, "call simulate M5 curTick()=%lu m_fooTicks=%lu\n", 
						curTick(), m_fooTicks );
    ::SimLoopExitEvent* exitEvent = simulate( m_fooTicks );
    DBGX( 5, "simulate returned M5 curTick() %lu\n", curTick() );
    m_fooTicks = m_m5ticksPerSSTclock;

    if( exitEvent->getCode() != 256 )  {
        // for fast-forwarding to reache the ROI at a faster speed in real 
        // benchmarks the exitcode is set to 100 to distinguish from regular 
        // exit
        if ( exitEvent->getCode() == 100 ){
	        std::cout << "I entered getCode = 100" << std::endl;

	        FastForwarding_flag=true;
#if 0
	        SimObject::SST_FF();
#endif
	        if(simulate()->getCode()>=0){
		        unregisterExit();
	        }
        } else {
            // bug what if we didn't call registerExit()
            unregisterExit();
            INFO( "exiting: curTick()=%lu cause=`%s` code=%d\n", curTick(),
                exitEvent->getCause().c_str(), exitEvent->getCode() );
        }
        if ( !m_statFile.empty() ) {
            libgem5::DumpStats(m_statFile);
        }
        return true;
    }
    delete exitEvent;
    return false;
}

int SST::M5::M5::Finish()
{
    #ifdef M5_WITH_POWER
    Finish_Power();
    #endif
   
    return 0;
}


