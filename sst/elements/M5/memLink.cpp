#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <memLink.h>
#include <debug.h>
#include <m5.h>
#include <memEvent.h>
#include <paramHelp.h>

MemLink::MemLink( const MemLinkParams *p ) :
    MemObject( p ),
    m_comp( p->m5Comp ),
    m_port( NULL ),
    m_range( p->range )
{
    DBGX(2,"name=`%s` linkName=`%s`\n", name().c_str(), p->linkName.c_str());

    m_link = p->m5Comp->configureLink( p->linkName, "1ns",
        new SST::Event::Handler<MemLink>( this, &MemLink::eventHandler ) );

    assert( m_link );

    printf("FIXME MemLink::MemLink()\n");
    abort();
    // Fix me! We can't reset the M5 componets time base
    //m_tc = p->m5Comp->registerTimeBase("1ns");
    //assert( m_tc );
    //m_link->setDefaultTimeBase(m_tc);
}

Port *MemLink::getPort(const std::string &if_name, int idx )
{
    DPRINTFN("getPort: if_name=`%s` idx=%d\n",if_name.c_str(), idx );
    assert ( ! m_port );
    return m_port = new LinkPort( name() + ".linkPort", this );
}

void MemLink::init( void )
{
    DBGX(2,"\n" );
    m_port->sendStatusChange( Port::RangeChange );
}

MemLink* MemLink::create( std::string name, M5* comp, Port* port, const SST::Params& params )
{
    MemLinkParams& memLinkParams = *new MemLinkParams;

    DBGC(2,"object name `%s` link name `%s`\n",
                name.c_str(), params.find_string("name").c_str());

    memLinkParams.name = name + ".link-" + params.find_string("name");
    memLinkParams.m5Comp = comp;
    memLinkParams.linkName = params.find_string("name");

    INIT_HEX( memLinkParams, params, range.start );
    INIT_HEX( memLinkParams, params, range.end );

    MemLink* memLink = new MemLink( &memLinkParams );

    Port* linkPort = memLink->getPort( "" );
    assert( linkPort );

    linkPort->setPeer( port ); 
    port->setPeer( linkPort );

    return memLink;
}

void MemLink::getAddressRanges(AddrRangeList &resp, bool &snoop ) {
    DPRINTFN("getAddressRanges:\n");
    snoop = false;
    resp.clear();
    resp.push_back( RangeSize(m_range.start, m_range.size()));
}

bool MemLink::send( SST::Event* event )
{
    // Note that we are not throttling data on this link
    DPRINTFN("%s() \n",__func__);
    m_link->Send( event );
    return true;
}

void MemLink::eventHandler( SST::Event* e )
{
    SST::Cycle_t now = 
                m_tc->convertToCoreTime( m_comp->getCurrentSimTime(m_tc) );

    if ( m_comp->catchup( now ) ) { 
        DBGX(3,"catchup() says we're exiting\n");
        return; 
    }

    MemEvent* event = static_cast<MemEvent*>(e);
    PacketPtr pkt = event->M5_Packet();

    DBGX( 3,"SST-time=%lu `%s` %#lx\n", now,
        pkt->cmdString().c_str(), (long) pkt->getAddr() );

    DPRINTFN("eventHandler: `%s` %#lx\n", pkt->cmdString().c_str(),
                    (long) pkt->getAddr() );
 
    switch ( event->type() ) {
        case MemEvent::Functional:
            m_port->sendFunctional( pkt );
            break;

        case MemEvent::Timing:
            m_port->sendTiming( pkt );
        break;
    }

    DBGX(3,"call arm\n");
    m_comp->arm( now );

    delete event;
}
BOOST_CLASS_EXPORT(MemEvent);
