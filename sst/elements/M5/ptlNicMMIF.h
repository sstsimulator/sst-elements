#ifndef _ptlNicMMIF_h
#define _ptlNicMMIF_h

#include <dev/io_device.hh>
#include "portals4/ptlNic/cmdQueue.h"

#include <debug.h>

using namespace std;
class M5;

struct PtlNicMMIFParams : public DmaDeviceParams
{
    M5*  m5Comp; 
    Addr startAddr;
    int  id;
    int  clockFreq;
};

class PtlNicMMIF : public DmaDevice 
{
    class DmaEvent : public ::Event
    {
      public:
        DmaEvent( PtlNicMMIF* obj, void* key) : 
            m_obj( obj ),
            m_key( key ) {}

        void process() {
            m_obj->process( m_key );
        }
        const char *description() const {
            return "DMA event completed";
        }
      private:
        PtlNicMMIF* m_obj;
        void*       m_key;
    };

    class ClockEvent : public ::Event
    {
      public:
        ClockEvent( PtlNicMMIF* obj, int interval ) : 
            m_obj( obj ), m_interval( interval)  {
            m_obj->schedule( this, curTick() + m_interval );
        }

        void process() {
            m_obj->clock();
            m_obj->schedule( this, curTick() + m_interval );
        }

        const char *description() const {
            return "clock";
        }

      private:
        PtlNicMMIF* m_obj;
        int         m_interval;
    };

  public:
    typedef PtlNicMMIFParams Params;
    PtlNicMMIF( const Params* p );
    virtual ~PtlNicMMIF();

    virtual Tick write(Packet*);
    virtual Tick read(Packet*);
    virtual void addressRanges(AddrRangeList&); 

  private:

    void process( void* );
    void clock( );
    void ptlCmdRespHandler( SST::Event* );
    void dmaHandler( SST::Event* );

    cmdQueue_t          m_cmdQueue;
    uint64_t            m_startAddr;
    uint64_t            m_endAddr;
    M5*                 m_comp;
    static const char*  m_cmdNames[];
    SST::Link*          m_cmdLink;
    SST::Link*          m_dmaLink;
    bool                m_blocked;
    ClockEvent*         m_clockEvent;
    std::deque< SST::Event* > m_dmaReadEventQ;
    std::deque< SST::Event* > m_dmaWriteEventQ;
};

const char * PtlNicMMIF::m_cmdNames[] = CMD_NAMES;

#endif
