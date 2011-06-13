#ifndef _ptlNicMMIF_h
#define _ptlNicMMIF_h

#include <dev/io_device.hh>
#include "cmdQueue.h"

#include <debug.h>

using namespace std;
class M5;

struct PtlNicMMIFParams : public DmaDeviceParams
{
    M5*  m5Comp; 
    Addr startAddr;
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

  public:
    typedef PtlNicMMIFParams Params;
    PtlNicMMIF( const Params* p );
    virtual ~PtlNicMMIF();

    virtual Tick write(Packet*);
    virtual Tick read(Packet*);
    virtual void addressRanges(AddrRangeList&); 

  private:

    void process( void* );
    void ptlCmdRespHandler( SST::Event* );
    void dmaHandler( SST::Event* );

    cmdQueue_t          m_cmdQueue;
    uint64_t            m_startAddr;
    uint64_t            m_endAddr;
    M5*                 m_comp;
    static const char*  m_cmdNames[];
//    SST::TimeConverter* m_tc;
    SST::Link*          m_cmdLink;
    SST::Link*          m_dmaLink;
    bool                m_blocked;
};

const char * PtlNicMMIF::m_cmdNames[] = CMD_NAMES;

#endif
