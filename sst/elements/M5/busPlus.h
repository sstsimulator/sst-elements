#ifndef _busPlus_h
#define _busPlus_h

#include <sst_config.h>
#include <sst/core/component.h>
#include <mem/bus.hh>
#include <debug.h>
#include <memEvent.h>

using namespace SST;
using namespace std;

struct BusPlusParams;
class M5;

class BusPlus : public Bus
{
    class LinkPort : public Port
    {
      public:
        LinkPort( const string &name, BusPlus* obj ) :
            Port( name, obj ),
            m_obj( obj )
        {
        }

        void sendFunctional(PacketPtr pkt) { 
	    DPRINTFN("%s()\n",__func__);
	    Port::sendFunctional( pkt );
	    delete pkt;
	}

        bool sendTiming(PacketPtr pkt) { 
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
	    return m_obj->send( new MemEvent( pkt ) );
	}

        virtual Tick recvAtomic(PacketPtr pkt)
        { cerr << WHERE <<  "abort" << endl; exit(1); }

        virtual void recvFunctional(PacketPtr pkt) { 
	    DPRINTFN("%s()\n",__func__);
	    if ( ! pkt->cmd.isWrite() ) {
                _error( Port2Link, "read is not supported\n" );
            }
	    m_obj->send( new MemEvent( pkt, MemEvent::Functional ) );
	}

        virtual void recvRetry() {
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
        { m_obj->getAddressRanges( resp, snoop ); }

      private:
        BusPlus* m_obj;

        std::deque< PacketPtr > m_deferred;
    };

  public:
    BusPlus( M5* comp, const BusPlusParams* p );

  private:

    bool send( MemEvent* );

    void getAddressRanges(AddrRangeList &resp, bool &snoop );

    void eventHandler( SST::Event* );

    void init( void );

    Link*            m_link;
    TimeConverter*   m_tc;
    M5*              m_comp; 

    Addr	     m_startAddr; 
    Addr	     m_endAddr; 

    struct linkInfo {
	LinkPort* linkPort;
        Port*     busPort;
    };    
    vector<linkInfo> m_port;
};

#endif
