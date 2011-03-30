#ifndef _memLink_h
#define _memLink_h

#include <sst_config.h>
#include <sst/core/component.h>

#include <mem/mem_object.hh>
#include <memEvent.h>

class M5;

struct MemLinkParams : public MemObjectParams
{
    M5*             m5Comp;
    Range< Addr >   range;
    std::string     linkName;
};

class SST::Link;
class SST::TimeConverter;
class MemEvent;

class MemLink : public MemObject
{
    class LinkPort : public Port
    {
      public:
        LinkPort( const std::string &name, MemLink* obj ) :
            Port( name, obj )
        {
        }

        void sendFunctional(PacketPtr pkt)
        {
            DPRINTFN("%s()\n",__func__);
            Port::sendFunctional( pkt );
            delete pkt;
        }

        bool sendTiming(PacketPtr pkt)
        {
            if ( pkt->memInhibitAsserted() ) {
                DPRINTFN("%s() memInhbitAssert()\n",__func__);
                delete pkt;
                return true;
            }
            if ( ! m_deferred.empty() || ! Port::sendTiming( pkt ) ) {
                DPRINTFN("%s() deffered\n",__func__);
                m_deferred.push_back( pkt );
            } else {
                DPRINTFN("%s()\n",__func__);
            }
            return true;
        }

      protected:
        virtual bool recvTiming(PacketPtr pkt) {
            DPRINTFN("%s()\n",__func__);
            return static_cast<MemLink*>(owner)->send( new MemEvent( pkt ) );
        }

        virtual Tick recvAtomic(PacketPtr pkt) { panic("??"); }

        virtual void recvFunctional(PacketPtr pkt)
        {
            DPRINTFN("%s()\n",__func__);

            if ( ! pkt->cmd.isWrite() ) { panic( "read is not supported" ); }

            static_cast<MemLink*>(owner)->send(
                    new MemEvent( pkt, MemEvent::Functional ) );
        }
        virtual void recvRetry()
        {
            while ( ! m_deferred.empty() ) {
                if ( ! Port::sendTiming( m_deferred.front() ) ) {
                    break;
                }
                DPRINTFN("%s()\n",__func__);
                m_deferred.pop_front();
            }
        }

        virtual void recvStatusChange(Status status) {}

        virtual void getDeviceAddressRanges(AddrRangeList &resp, bool &snoop)
        { static_cast<MemLink*>(owner)->getAddressRanges( resp, snoop ); }

      private:

        std::deque< PacketPtr > m_deferred;
    };

  public:
    MemLink( const MemLinkParams * );
    virtual Port *getPort(const std::string &if_name, int idx = -1);
    static MemLink* create( std::string name, M5*, Port*, const SST::Params& ); 

  private:
    void init( void );
    bool send( SST::Event* );
    void eventHandler( SST::Event* );
    void getAddressRanges(AddrRangeList &resp, bool &snoop );

    M5*                   m_comp;
    SST::Link*            m_link;
    SST::TimeConverter*   m_tc;
    LinkPort*             m_port;
    Range< Addr >         m_range;
};

#endif
