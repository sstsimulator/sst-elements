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
    INIT_STR( params, sstParams, megsOfMemory );

    params.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );
    params.exeParams = sstParams.find_prefix_params("exe");

    return new DRAMSimWrap( &params );
}

DRAMSimWrap::DRAMSimWrap( const Params* p ) :
    PhysicalMemory( p ),
    m_waitRetry( false ),
    m_port( NULL ),
    m_comp( p->m5Comp ),
    m_dbg( *new Log< DRAMSIMC_DBG >( cerr, "DRAMSimC::", false ) ),
    m_log( *new Log< >( cout, "INFO DRAMSimC: ", false ) )
{
    unsigned megsOfMemory = 4096;
    DBGX( 2, "name=`%s`\n", name().c_str() );

    if ( p->info.compare( "yes" ) == 0 ) {
        m_log.enable();
    }

    if ( p->debug.compare( "yes" ) == 0 ) {
        m_dbg.enable();
    }

    loadMemory( name(), this, p->exeParams );

    string deviceIniFilename = p->pwd + "/" + p->deviceIniFilename;
    string systemIniFilename = p->pwd + "/" + p->systemIniFilename;

    stringstream(p->megsOfMemory) >> megsOfMemory;

    m_log.write("device ini %s\n",deviceIniFilename.c_str());
    m_log.write("system ini %s\n",systemIniFilename.c_str());

    m_log.write("freq %s\n",p->frequency.c_str());
    m_log.write("megsOfMemory %lu\n",megsOfMemory);

    m_tc = m_comp->registerClock( p->frequency,
            new SST::Clock::Handler<DRAMSimWrap>(this, &DRAMSimWrap::clock) );

    assert( m_tc );
    m_log.write("period %ld\n",m_tc->getFactor());

    m_memorySystem = new DRAMSim::MemorySystem(0, deviceIniFilename,
                    systemIniFilename, "", "", megsOfMemory );

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

Port * DRAMSimWrap::getPort(const std::string &if_name, int idx )
{
    PRINTFN("%s: if_name=`%s` idx=%d\n", __func__, if_name.c_str(), idx );

    if (if_name == "functional") {
        return new MemoryPort(csprintf("%s-functional", name()), this);
    }

    assert( ! m_port );

    if (if_name != "port") {
        panic("DRAMSimWrap::getPort: unknown port %s requested", if_name);
    }

    return m_port = new MemPort( name() + "." + if_name, this );
}

void DRAMSimWrap::init()
{
    if ( ! m_port ) {
        fatal("DRAMSimWrap object %s is unconnected!", name());
    }

    m_port->sendStatusChange(Port::RangeChange);
}

bool DRAMSimWrap::recvTiming(PacketPtr pkt)
{
    PRINTFN("%s: %s %#lx\n", __func__, pkt->cmdString().c_str(),
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
    PRINTFN("%s:\n",__func__);
    assert( ! m_readyQ.empty() );

    m_waitRetry = false;
    sendTry();
}

void DRAMSimWrap::sendTry()
{
    while ( ! m_waitRetry && ! m_readyQ.empty() ) {
        if ( ! m_port->sendTiming( m_readyQ.front() )  ) {
            PRINTFN("%s: sendTiming failed\n",__func__);
            m_waitRetry = true;
        } else {
            PRINTFN("%s: sendTiming succeeded\n",__func__);
            m_readyQ.pop_front();
        }
    }
}

void DRAMSimWrap::readData(uint id, uint64_t addr, uint64_t clockcycle)
{
    PRINTFN("%s: id=%d addr=%#lx clock=%lu\n",__func__,id,addr,clockcycle);

    assert( ! m_readQ.empty() );

    m_readQ.front().second += JEDEC_DATA_BUS_BITS; 

    PacketPtr pkt = m_readQ.front().first;

    if ( m_readQ.front().second >= pkt->getSize() ) {
        PhysicalMemory::doAtomicAccess( pkt );
        m_readyQ.push_back(pkt);
        m_readQ.pop_front();
    } 
}

void DRAMSimWrap::writeData(uint id, uint64_t addr, uint64_t clockcycle)
{
    PRINTFN("%s: id=%d addr=%#lx clock=%lu\n",__func__,id,addr,clockcycle);

    assert( ! m_writeQ.empty() );

    m_writeQ.front().second += JEDEC_DATA_BUS_BITS; 

    PacketPtr pkt = m_writeQ.front().first;

    if ( m_writeQ.front().second >= pkt->getSize() ) {
        PhysicalMemory::doAtomicAccess( pkt );
        if ( pkt->needsResponse() ) {
            m_readyQ.push_back(pkt);
        } else {
            delete pkt;
        }
        m_writeQ.pop_front();
    } 
}

bool DRAMSimWrap::clock( SST::Cycle_t cycle )
{
    SST::Cycle_t now = m_tc->convertToCoreTime( cycle );

    if ( m_comp->catchup( ) ) {
        return true;
    }

    MS_CAST( m_memorySystem )->update();

    sendTry();

    while ( ! m_recvQ.empty() ) {
        PacketPtr pkt = m_recvQ.front().first;
        int ret;
        uint64_t addr = pkt->getAddr() + m_recvQ.front().second;
        addr &= ~( JEDEC_DATA_BUS_BITS - 1 );

        if ( ! MS_CAST( m_memorySystem )->addTransaction( 
                        pkt->isWrite(), addr) ) 
        {
            break;
        }

        PRINTFN("%s: added trans, cycle=%lu addr=%#lx\n", 
                                                __func__,cycle, addr );

        m_recvQ.front().second += JEDEC_DATA_BUS_BITS;

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
