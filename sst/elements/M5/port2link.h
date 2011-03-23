#ifndef _port2link_h
#define _port2link_h

#include <sst_config.h>
#include <sst/core/component.h>
#include <base/trace.hh>

// M5 includes
#include <mem/mem_object.hh>

using namespace SST;

#define _error(name, fmt, args...) \
{\
fprintf(stderr,"%s::%s():%i:FAILED: " fmt, #name, __FUNCTION__, __LINE__, ## args);\
exit(-1); \
}

#undef DPRINTFN
#define DPRINTFN(...) do {                                      \
    if ( Trace::enabled )                                       \
        Trace::dprintf(curTick, name(), __VA_ARGS__);           \
} while (0)

struct Port2LinkParams;
class M5;

#include <memEvent.h>

class Port2Link : public MemObject
{
    class IdleEvent : public ::Event
    {
        Port2Link * m_link;

      public:
        IdleEvent( Port2Link * link ) :
            m_link( link ) { 
        }
        void process() {
            m_link->recvRetry( -1 );
        }
        const char *description() const {
            return "link became available";
        }
    };

    class MemObjPort : public Port
    {
      public:
        MemObjPort(const std::string &_name, Port2Link* object ) : 
            Port( _name, object ), m_object( *object )
        { }

      protected:
        virtual bool recvTiming(PacketPtr pkt) 
        { return m_object.recvTiming( pkt ); }

        virtual Tick recvAtomic(PacketPtr pkt) 
        { _error(Port2Link::MemObjPort,"\n"); }

        virtual void recvFunctional(PacketPtr pkt)
        { m_object.recvFunctional( pkt ); }

        virtual void recvStatusChange(Status status) 
        { DPRINTFN("Port2Link::MemObjPort::recvStatusChange()\n"); }

        virtual void recvRetry( ) 
        { m_object.recvRetry( 0 ); }

        virtual void getDeviceAddressRanges(AddrRangeList &resp, bool &snoop) 
        { m_object.getAddressRanges( resp, snoop ); }

      private:
        Port2Link& m_object;
    };

  public:
    Port2Link( M5* comp, const Port2LinkParams *p );

  private:

  public:
    // SST function
    void eventHandler( SST::Event* );

    // M5 function
    virtual void init();
    virtual Port *getPort(const std::string &if_name, int idx = -1);
    bool recvTiming(PacketPtr pkt);
    void recvFunctional(PacketPtr pkt );
    void recvRetry( int id );
    void getAddressRanges(AddrRangeList &resp, bool &snoop );
    Tick calcPacketTiming(PacketPtr pkt);
    void occupyLink(Tick until);

    std::deque< PacketPtr > m_deferred;
    std::deque< PacketPtr > m_deferredPkt;
    Link*   m_link;

    MemObjPort*     m_memObjPort;
    TimeConverter  *m_tc;
    M5*             m_comp;
    int             m_dbgFlag;

    Tick            tickNextIdle;
    int             clock;
    int             width;
    int             headerCycles;
    int             defaultBlockSize;
    IdleEvent       linkIdleEvent;
    bool            inRetry;
};
#endif
