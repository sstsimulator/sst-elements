#ifndef _DRAMSimWrap_h
#define _DRAMSimWrap_h

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
    string frequency;
    string systemIniFilename;
    string deviceIniFilename;
    string info;
    string debug;
    string pwd;
    string printStats;
};

class DRAMSimWrap : public PhysicalMemory {

    class MemoryPort : public Port 
    {
      public:
        MemoryPort( const std::string &name, DRAMSimWrap* obj ) :
            Port( name, obj ),
            m_obj( obj )
        {
        } 
      protected:
        virtual bool recvTiming(PacketPtr pkt) {
            m_obj->recvTiming( pkt );
        }

        virtual Tick recvAtomic(PacketPtr pkt) 
        { cerr << WHERE <<  "abort" << endl; exit(1); }

        virtual void recvFunctional(PacketPtr pkt) {
            m_obj->doFunctionalAccess( pkt );
        }

        virtual void recvStatusChange(Status status) { }

        virtual void recvRetry( ) { 
            m_obj->recvRetry( ); 
        }

      private:
        DRAMSimWrap* m_obj;
    }; 

  public:
    typedef DRAMSimWrapParams Params;
    DRAMSimWrap( M5*, const Params* p );

    void init() {}

    bool recvTiming(PacketPtr pkt);
    void doFunctionalAccess(PacketPtr pkt);
    void recvRetry();

    virtual Port * getPort(const std::string &if_name, int idx = -1);

  private:
    virtual ~DRAMSimWrap( );
    const DRAMSimWrap &operator=( const Params& p );

    bool clock( SST::Cycle_t );
    void readData(unsigned int id, uint64_t addr, uint64_t clockcycle);
    void writeData(unsigned int id, uint64_t addr, uint64_t clockcycle);

    std::vector<MemoryPort*>    m_ports;

    std::deque<std::pair<PacketPtr,int> >       m_recvQ;
    std::deque<PacketPtr >                      m_readyQ;
    std::deque<std::pair<PacketPtr,int> >       m_readQ;
    std::deque<std::pair<PacketPtr,int> >       m_writeQ;

    MemoryPort*     m_funcPort;
    unsigned char*  m_pmemAddr; 

    // define this as void so we don't have to pull in DRAMSim header files
    void*           m_memorySystem;
    bool            m_waitRetry;
    M5*             m_comp;

    SST::TimeConverter  *m_tc;

    Log< DRAMSIMC_DBG >&    m_dbg;
    Log<>&                  m_log;
};

#endif
