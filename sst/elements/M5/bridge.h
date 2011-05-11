
#ifndef _bridge_h
#define _bridge_h

#include <sst_config.h>
#include <sst/core/component.h>

#include <mem/mem_object.hh>

#include <debug.h>

class M5;

struct BridgeParams : public MemObjectParams
{
    Range< Addr >   range[2];
    Addr            mapTo[2];
};

class Bridge : public MemObject
{
    class BridgePort : public Port
    {
      public:
        BridgePort( const std::string &name, Bridge* obj, int idx ) :
            Port( name, obj ),
            m_idx( idx )
        {}   

      private:
        virtual void getDeviceAddressRanges(AddrRangeList &resp, bool &snoop)
        {
            static_cast<Bridge*>(owner)->getAddressRanges( resp, snoop, m_idx );
        }

        virtual bool recvTiming(PacketPtr pkt) {
            DBGX(2,"\n");
            return static_cast<Bridge*>(owner)->recvTiming( pkt, m_idx );
        }
        virtual Tick recvAtomic(PacketPtr pkt) { panic("??"); }
        virtual void recvFunctional(PacketPtr pkt) {}
        virtual void recvStatusChange(Status status) {}
    
        int m_idx;
#if 0
        void sendFunctional(PacketPtr pkt)
        bool sendTiming(PacketPtr pkt)
        virtual void recvRetry()
#endif
    };

  public:   
    Bridge( const BridgeParams * );
    void init( void );

    virtual Port *getPort(const std::string &if_name, int idx = -1);
    void getAddressRanges(AddrRangeList &resp, bool &snoop, int idx );
    bool recvTiming( PacketPtr, int );

  private:
    BridgePort*     m_ports[2];
    Range< Addr >   m_range[2];
    Addr            m_offset[2];
};

#endif
