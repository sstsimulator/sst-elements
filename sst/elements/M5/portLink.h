#ifndef __portLink_h
#define __portLink_h

#include <sst_config.h>
#include <sst/core/component.h>

#include <debug.h>
#include <m5.h>
#include <dll/gem5dll.hh>
#include <rawEvent.h>

class PortLink {
  public:
    inline PortLink( M5&, Gem5Object_t&, const SST::Params& );
  private:
    inline void eventHandler( SST::Event* );     

    inline  bool recv( void* data, size_t len ) {
        DBGC(2,"\n");
        
        // Note we are not throttling events
        m_link->Send( new RawEvent( data, len ) );
        return true;
    }

    static inline  bool recv( void *obj, void *data, size_t len ) {
        return ((PortLink*)obj)->recv( data, len );
    }

    SST::Link*      m_link;
    M5&             m_comp;
    MsgEndPoint*    m_gem5EndPoint;
    MsgEndPoint     m_myEndPoint;
};

PortLink::PortLink( M5& comp, Gem5Object_t& obj, const SST::Params& params ) :
    m_comp( comp ),
    m_myEndPoint( this, recv )
{
    bool snoop = false;
    std::string name = params.find_string("name");
    assert( ! name.empty() );
    std::string portName = params.find_string("portName");
    assert( ! portName.empty() );
    int idx = params.find_integer( "portIndex" );
    uint64_t addrStart = 0, addrEnd = -1;

    if ( params.find_string("snoop").compare("true") == 0 ) {
        snoop = true;
    }

    if ( params.find("range.start") != params.end() ) {
        addrStart = params.find_integer("range.start");
    }
    if ( params.find("range.end") != params.end() ) {
        addrEnd = params.find_integer("range.end");
    }

    DBGC(2,"%s portName=%s index=%d\n", name.c_str(), portName.c_str(), idx );

    m_link = comp.configureLink( name,
        new SST::Event::Handler<PortLink>( this, &PortLink::eventHandler ) );
    assert( m_link );

    m_gem5EndPoint = GetPort(obj, m_myEndPoint, snoop, 
                            addrStart, addrEnd, portName, idx ); 
    assert( m_gem5EndPoint );
}

void PortLink::eventHandler( SST::Event *e )
{
    DBGC(2,"\n");

    RawEvent *event = static_cast<RawEvent*>( e ); 
    
    if ( ! m_gem5EndPoint->send( event->data(), event->len() ) ) {
        // What if Gem5 refuses to take this event? 
        // We need to check for failure and respond to retries
        abort();
    };

    delete e;
}

#endif
