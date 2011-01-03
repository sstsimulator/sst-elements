#include <sys/mman.h>

#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <m5.h>
#include <DRAMSimWrap.h>
#include <log.h>
#include <paramHelp.h>

#include <loadMemory.h>

// DEBUG might be defined for M5, include DRAMSim/MemorySystem.h last
// and undefined DEBUG because DRAMSim defines it
#undef DEBUG
#include <MemorySystem.h>

using namespace SST;
using namespace std;

#define _dbg( fmt, args...)\
   if ( Trace::enabled ) \
      fprintf(stderr,"%7lu: %s: " fmt, (unsigned long)curTick, \
                                        name().c_str(), ## args )

#define MS_CAST( x ) static_cast<DRAMSim::MemorySystem*>(x)

extern "C" {
    SimObject* create_DRAMSimWrap( void*, string name, Params& sParams );
}

SimObject* create_DRAMSimWrap( void* comp, string name, Params& sstParams )
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

    PhysicalMemory* physmem = 
                        new DRAMSimWrap( static_cast<M5*>(comp), &params );

    loadMemory( name, physmem, sstParams );

    return physmem;
}

DRAMSimWrap::DRAMSimWrap( M5* comp, const Params* p ) :
    PhysicalMemory( p ),
    m_funcPort( NULL ),
    m_waitRetry( false ),
    m_comp( comp ),
    m_dbg( *new Log< DRAMSIMC_DBG >( cerr, "DRAMSimC::", false ) ),
    m_log( *new Log< >( cout, "INFO DRAMSimC: ", false ) )
{
    if ( p->info.compare( "yes" ) == 0 ) {
        m_log.enable();
    }

    if ( p->debug.compare( "yes" ) == 0 ) {
        m_dbg.enable();
    }

    for ( int i = 0; i < m_ports.size(); i++ )
         m_ports[i] = NULL;

    int map_flags = MAP_ANON | MAP_PRIVATE;
    m_pmemAddr = (uint8_t *)mmap(NULL, params()->range.size(),
                               PROT_READ | PROT_WRITE, map_flags, -1, 0);

    assert( m_pmemAddr );

    string deviceIniFilename = p->pwd + "/" + p->deviceIniFilename;
    string systemIniFilename = p->pwd + "/" + p->systemIniFilename;

    m_log.write("device ini %s\n",deviceIniFilename.c_str());
    m_log.write("system ini %s\n",systemIniFilename.c_str());

    m_log.write("freq %s\n",p->frequency.c_str());

    m_tc = comp->registerClock( p->frequency,
            new SST::Clock::Handler<DRAMSimWrap>(this, &DRAMSimWrap::clock) );

    assert( m_tc );
    m_log.write("period %ld\n",m_tc->getFactor());

    m_memorySystem = new DRAMSim::MemorySystem(0, deviceIniFilename,
                    systemIniFilename, "", "");

    assert( m_memorySystem );

    DRAMSim::Callback<DRAMSimWrap, void, uint, uint64_t,uint64_t >* readDataCB;
    DRAMSim::Callback<DRAMSimWrap, void, uint, uint64_t,uint64_t >* writeDataCB;

    readDataCB = new DRAMSim::Callback
        < DRAMSimWrap, void, uint, uint64_t,uint64_t > 
                        (this, &DRAMSimWrap::readData);

    assert( readDataCB );

    writeDataCB = new DRAMSim::Callback
        < DRAMSimWrap, void, uint, uint64_t,uint64_t >
                        (this, &DRAMSimWrap::writeData);

    assert( writeDataCB );

    MS_CAST( m_memorySystem )->RegisterCallbacks(readDataCB, writeDataCB, NULL);
}

DRAMSimWrap::~DRAMSimWrap()
{
    for ( int i = 0; i < m_ports.size(); i++ ) {
        if ( m_ports[i] ) {
            delete m_ports[i];
        }
    }
    delete &m_log;
    delete &m_dbg;
    delete m_funcPort;
}

Port * DRAMSimWrap::getPort( const string &if_name, int idx ) 
{
    DBGX( 1, "name=`%s` idx=%d\n", if_name.c_str(),idx);
    if ( if_name == "functional") {
        if ( m_funcPort ) {
            { cerr << WHERE <<  "abort" << endl; exit(1); }
        }
        return m_funcPort =
                    new MemoryPort(csprintf("%s-functional", name()), this);
    }

    if ( idx + 1 > m_ports.size() ) {
        m_ports.resize( idx + 1 );
    }

    if ( m_ports[idx] ) {
        cerr << WHERE <<  "abort" << endl; exit(1);
    }

    return m_ports[idx] =
                        new MemoryPort(csprintf("%s-functional", name()), this);
}

void DRAMSimWrap::doFunctionalAccess(PacketPtr pkt)
{
    Addr addr = pkt->getAddr(); 
    _dbg("doFunctionalAccess: %s addr=%#lx size=%i \n",
                    pkt->cmdString().c_str(), (long) addr, pkt->getSize() );

    uint8_t *hostAddr = m_pmemAddr + pkt->getAddr() - start();

    assert( ( pkt->getAddr() >= start() ) );
    assert( ( pkt->getAddr() + pkt->getSize() <= start() + size() ) );

    if ( pkt->isWrite() ) {
        memcpy(hostAddr, pkt->getPtr<uint8_t>(), pkt->getSize());
    } else {
        memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
    }
    pkt->makeAtomicResponse();
}

bool DRAMSimWrap::recvTiming(PacketPtr pkt)
{
    DBGX( 3, "%s %#lx\n", pkt->cmdString().c_str(),
                                        (long)pkt->getAddr());
    _dbg("recvTiming: %s %#lx\n", pkt->cmdString().c_str(),
                                        (long)pkt->getAddr());
    m_recvQ.push_back( make_pair( pkt, 0 ) );
    return true;
}

void DRAMSimWrap::recvRetry()
{
    m_waitRetry = false;
}

void DRAMSimWrap::readData(uint id, uint64_t addr, uint64_t clockcycle)
{
    DBGX(3,"id=%d addr=%#lx clock=%lu\n",id,addr,clockcycle);

    m_readQ.front().second += JEDEC_DATA_BUS_WIDTH; 

    PacketPtr pkt = m_readQ.front().first;

    if ( m_readQ.front().second == pkt->getSize() ) {
        doFunctionalAccess(pkt);
        m_readyQ.push_back(pkt);
        m_readQ.pop_front();
    } 
}

void DRAMSimWrap::writeData(uint id, uint64_t addr, uint64_t clockcycle)
{
    DBGX(3,"id=%d addr=%#lx clock=%lu\n",id,addr,clockcycle);

    m_readQ.front().second += JEDEC_DATA_BUS_WIDTH; 

    PacketPtr pkt = m_writeQ.front().first;

    if ( m_writeQ.front().second == pkt->getSize() ) {
        doFunctionalAccess(pkt);
        m_readyQ.push_back(pkt);
        m_writeQ.pop_front();
    } 
}

bool DRAMSimWrap::clock( SST::Cycle_t cycle )
{
    Cycle_t now = m_tc->convertToCoreTime( cycle );
    MS_CAST( m_memorySystem )->update();

    // calling sendTiming can change the state of the M5 queue, 
    // we need to bring it up to the current time and then arm it 
    m_comp->catchup( now ); 

    while ( ! m_waitRetry && ! m_readyQ.empty() ) {
        if ( ! m_ports[0]->sendTiming( m_readyQ.front() )  ) {
            DBGX(3,"sendTiming failed, clock=%lu\n",cycle);
            m_waitRetry = true;
            break;
        }
        DBGX(3,"sendTiming succeeded, clock=%lu\n",cycle);
        m_readyQ.pop_front();
    }

    m_comp->arm( now );

    while ( ! m_recvQ.empty() ) {
        PacketPtr pkt = m_recvQ.front().first;
        int ret;
        uint64_t addr = pkt->getAddr() + m_recvQ.front().second;

        if ( ! MS_CAST( m_memorySystem )->addTransaction( 
                        pkt->isWrite(), addr ) ) 
        {
            break;
        }

        DBGX(3,"added trans, cycle=%lu addr=%#lx\n", cycle, addr );

        m_recvQ.front().second += 8;

        if ( m_recvQ.front().second == pkt->getSize() ) {
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
