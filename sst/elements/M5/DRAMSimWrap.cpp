#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <DRAMSimWrap.h>
#include <memLink.h>
#include <m5.h>
#include <paramHelp.h>
#include <loadMemory.h>

// DEBUG might be defined for M5, include DRAMSim/MemorySystem.h last
// and undefined DEBUG because DRAMSim defines it
#undef DEBUG
#include <MemorySystem.h>

#define MS_CAST( x ) static_cast<DRAMSim::MemorySystem*>(x)

extern "C" {
    SimObject* create_DRAMSimWrap( SST::Component*, std::string, SST::Params& );
}

SimObject* create_DRAMSimWrap( SST::Component* comp, string name,
                                                    SST::Params& sstParams )
{
    DRAMSimWrap::Params& params   = *new DRAMSimWrap::Params;

    params.name = name;

    INIT_HEX( params, sstParams, range.start );
    INIT_HEX( params, sstParams, range.end );
    INIT_STR( params, sstParams, info );
    INIT_STR( params, sstParams, debug );
    INIT_STR( params, sstParams, frequency );
    INIT_STR( params, sstParams, pwd );
    INIT_STR( params, sstParams, printStats);
    INIT_STR( params, sstParams, deviceIniFilename );
    INIT_STR( params, sstParams, systemIniFilename );

    params.linkName = sstParams.find_string( "link.name" );
    params.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );
    params.exeParams = sstParams.find_prefix_params("exe.");

    return new DRAMSimWrap( &params );
}

DRAMSimWrap::DRAMSimWrap( const Params* p ) :
    MemObject( p ),
    m_waitRetry( false ),
    m_comp( p->m5Comp ),
    m_dbg( *new Log< DRAMSIMC_DBG >( cerr, "DRAMSimC::", false ) ),
    m_log( *new Log< >( cout, "INFO DRAMSimC: ", false ) )
{
    DBGX( 2, "name=`%s`\n", name().c_str() );

    if ( p->info.compare( "yes" ) == 0 ) {
        m_log.enable();
    }

    if ( p->debug.compare( "yes" ) == 0 ) {
        m_dbg.enable();
    }

    setupMemLink( p );
    setupPhysicalMemory( p );

    string deviceIniFilename = p->pwd + "/" + p->deviceIniFilename;
    string systemIniFilename = p->pwd + "/" + p->systemIniFilename;

    m_log.write("device ini %s\n",deviceIniFilename.c_str());
    m_log.write("system ini %s\n",systemIniFilename.c_str());

    m_log.write("freq %s\n",p->frequency.c_str());

    m_tc = m_comp->registerClock( p->frequency,
            new SST::Clock::Handler<DRAMSimWrap>(this, &DRAMSimWrap::clock) );

    assert( m_tc );
    m_log.write("period %ld\n",m_tc->getFactor());

    m_memorySystem = new DRAMSim::MemorySystem(0, deviceIniFilename,
                    systemIniFilename, "", "");

    DRAMSim::Callback<DRAMSimWrap, void, uint, uint64_t,uint64_t >* readDataCB;
    DRAMSim::Callback<DRAMSimWrap, void, uint, uint64_t,uint64_t >* writeDataCB;

    readDataCB = new DRAMSim::Callback
        < DRAMSimWrap, void, uint, uint64_t,uint64_t > 
                        (this, &DRAMSimWrap::readData);

    writeDataCB = new DRAMSim::Callback
        < DRAMSimWrap, void, uint, uint64_t,uint64_t >
                        (this, &DRAMSimWrap::writeData);

    MS_CAST( m_memorySystem )->RegisterCallbacks(readDataCB, writeDataCB, NULL);
}

DRAMSimWrap::~DRAMSimWrap()
{
    delete &m_log;
    delete &m_dbg;
}


void DRAMSimWrap::doFunctionalAccess(PacketPtr pkt)
{
    DPRINTFN("doFunctionalAccess: %s addr=%#lx size=%i\n",
                    pkt->cmdString().c_str(), pkt->getAddr(), pkt->getSize() );

    m_memPort->sendFunctional(pkt);
}

bool DRAMSimWrap::recvTiming(PacketPtr pkt)
{
    DBGX( 3, "%s inval=%d resp=%d %#lx\n", pkt->cmdString().c_str(),
        pkt->isInvalidate(), pkt->needsResponse(), (long)pkt->getAddr());
    DPRINTFN("recvTiming: %s %#lx\n", pkt->cmdString().c_str(),
                                        (long)pkt->getAddr());

    if ( pkt->isRead() || pkt->isWrite() ) {
        m_recvQ.push_back( make_pair( pkt, 0 ) );
    } else {
        if ( pkt->needsResponse() ) {
            pkt->makeTimingResponse();
            m_readyQ.push_back( pkt );
        } else {
            delete pkt;
        }
    }
    return true;
}

void DRAMSimWrap::recvRetry()
{
    m_waitRetry = false;
}

void DRAMSimWrap::readData(uint id, uint64_t addr, uint64_t clockcycle)
{
    DBGX(3,"id=%d addr=%#lx clock=%lu\n",id,addr,clockcycle);

    assert( ! m_readQ.empty() );

    m_readQ.front().second += JEDEC_DATA_BUS_WIDTH; 

    PacketPtr pkt = m_readQ.front().first;

    if ( m_readQ.front().second >= pkt->getSize() ) {
        m_memPort->sendFunctional( pkt );
        m_readyQ.push_back(pkt);
        m_readQ.pop_front();
    } 
}

void DRAMSimWrap::writeData(uint id, uint64_t addr, uint64_t clockcycle)
{
    DBGX(3,"id=%d addr=%#lx clock=%lu\n",id,addr,clockcycle);

    assert( ! m_writeQ.empty() );

    m_writeQ.front().second += JEDEC_DATA_BUS_WIDTH; 

    PacketPtr pkt = m_writeQ.front().first;

    if ( m_writeQ.front().second >= pkt->getSize() ) {
        m_memPort->sendFunctional( pkt );
        m_readyQ.push_back(pkt);
        m_writeQ.pop_front();
    } 
}

bool DRAMSimWrap::clock( SST::Cycle_t cycle )
{
    SST::Cycle_t now = m_tc->convertToCoreTime( cycle );

    // calling sendTiming can change the state of the M5 queue, 
    // we need to bring it up to the current time and then arm it 
    if ( m_comp->catchup( now ) ) {
	    DBGX(3,"catchup() says we're exiting\n");
        return false;
    }

    MS_CAST( m_memorySystem )->update();

    while ( ! m_waitRetry && ! m_readyQ.empty() ) {
        if ( ! m_linkPort->sendTiming( m_readyQ.front() )  ) {
            DBGX(3,"sendTiming failed, clock=%lu\n",cycle);
            m_waitRetry = true;
            break;
        }
        DBGX(3,"sendTiming succeeded, clock=%lu\n",cycle);
        m_readyQ.pop_front();
    }

    DBGX(3,"call arm\n");
    m_comp->arm( now );

    while ( ! m_recvQ.empty() ) {
        PacketPtr pkt = m_recvQ.front().first;
        int ret;
        uint64_t addr = pkt->getAddr() + m_recvQ.front().second;
        addr &= ~( JEDEC_DATA_BUS_WIDTH - 1 );

        if ( ! MS_CAST( m_memorySystem )->addTransaction( 
                        pkt->isWrite(), addr) ) 
        {
            break;
        }

        DBGX(3,"added trans, cycle=%lu addr=%#lx\n", cycle, addr );

        m_recvQ.front().second += JEDEC_DATA_BUS_WIDTH;

        DBGX(3,"getAddr()=%#lx getSize()=%#lx %#lx\n",
			pkt->getAddr(), pkt->getSize(), m_recvQ.front().second );

        if ( m_recvQ.front().second >= pkt->getSize() ) {
            if ( pkt->isRead() ) {
                m_readQ.push_back( make_pair( pkt, 0 ) );
            } else {
                m_writeQ.push_back( make_pair( pkt, 0 ) );
            }
            m_recvQ.pop_front();
        }
    }

    return false;
}

void DRAMSimWrap::setupMemLink( const Params* p )
{
    m_linkPort = new LinkPort( "link", this );

    SST::Params params;
    params["name"]        = p->linkName;
    params["range.start"] = p->range.start;
    params["range.end"]   = p->range.end;

    m_link =  MemLink::create( name(), p->m5Comp, m_linkPort, params );
}

void DRAMSimWrap::setupPhysicalMemory( const Params* p )
{
    PhysicalMemoryParams& params  = *new PhysicalMemoryParams;

    params.range.start = p->range.start;
    params.range.end = p->range.end;
    params.latency = 0;
    params.latency_var = 0;
    params.null = false;
    params.zero = false;
   
    m_physmem = new PhysicalMemory( &params );

    m_memPort = new MemPort( "mem", this );
    Port* port = m_physmem->getPort( "port", 0 );

    port->setPeer( m_memPort );
    m_memPort->setPeer( port );

    loadMemory( name(), m_physmem, p->exeParams );
}

