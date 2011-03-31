#ifndef _DRAMSimWrap_h
#define _DRAMSimWrap_h

#include <sst_config.h>
#include <sst/core/component.h>

#include <mem/physical.hh>
#include <debug.h>
#include <deque>

#include <sst/core/timeConverter.h>

#ifndef DRAMSIMC_DBG
#define DRAMSIMC_DBG 1 
#endif

using namespace std;

class M5;

struct DRAMSimWrapParams : public PhysicalMemoryParams
{
    M5*         m5Comp;
    SST::Params exeParams;
    std::string frequency;
    std::string systemIniFilename;
    std::string deviceIniFilename;
    std::string info;
    std::string debug;
    std::string pwd;
    std::string printStats;
};

class DRAMSimWrap : public PhysicalMemory 
{
    class MemPort : public Port {
      public:
        MemPort( const std::string &name, DRAMSimWrap* obj ) :
            Port( name, obj )
        {
        } 
      protected:
        virtual bool recvTiming(PacketPtr pkt) {

            if (pkt->memInhibitAsserted()) {
                // snooper will supply based on copy of packet
                // still target's responsibility to delete packet
                DPRINTFN("recvTiming: %s %#lx memInhibitAssert()\n",
                        pkt->cmdString().c_str(), (long)pkt->getAddr());
                delete pkt;
                return true;
            }

            static_cast<DRAMSimWrap*>(owner)->recvTiming( pkt );
        }

        virtual Tick recvAtomic(PacketPtr pkt) 
        { cerr << WHERE <<  "abort" << endl; exit(1); }

        virtual void recvFunctional(PacketPtr pkt) {
            static_cast<DRAMSimWrap*>(owner)->doFunctionalAccess( pkt );
        }

        virtual void recvStatusChange(Status status) { 
            static_cast<DRAMSimWrap*>(owner)->recvStatusChange( status );
        }

        virtual void recvRetry( ) { 
            static_cast<DRAMSimWrap*>(owner)->recvRetry( ); 
        }

        virtual void getDeviceAddressRanges(AddrRangeList &resp, bool &snoop) {
            static_cast<DRAMSimWrap*>(owner)->getAddressRanges( resp, snoop ); 
        }
    };

  public:
    typedef DRAMSimWrapParams Params;
    DRAMSimWrap( const Params* p );

    virtual Port * getPort(const std::string &if_name, int idx = -1);

    void init();

  private:
    virtual ~DRAMSimWrap( );
    const DRAMSimWrap &operator=( const Params& p );

    bool recvTiming(PacketPtr pkt);
    void recvRetry();
    void sendTry();

    bool clock( SST::Cycle_t );
    void readData(unsigned int id, uint64_t addr, uint64_t clockcycle);
    void writeData(unsigned int id, uint64_t addr, uint64_t clockcycle);

    std::deque<std::pair<PacketPtr,int> >       m_recvQ;
    std::deque<PacketPtr >                      m_readyQ;
    std::deque<std::pair<PacketPtr,int> >       m_readQ;
    std::deque<std::pair<PacketPtr,int> >       m_writeQ;

    // define this as void so we don't have to pull in DRAMSim header files
    void*           m_memorySystem;
    bool            m_waitRetry;
    M5*             m_comp;
    MemPort*        m_port;

    SST::TimeConverter*     m_tc;

    Log< DRAMSIMC_DBG >&    m_dbg;
    Log<>&                  m_log;
};

#endif
