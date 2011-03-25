
#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <busPlus.h>
#include <paramHelp.h>
#include <m5.h>
#include <memEvent.h>

struct BusPlusParams : public BusParams
{
    std::string linkName;
    Range< Addr > range;
};

extern "C" {
    SimObject* create_BusPlus( void*, std::string name, Params& params );
}

SimObject* create_BusPlus( void* comp, std::string name, Params& params )
{
    BusPlusParams& busP = *new BusPlusParams;
    busP.name = name;

    INIT_HEX( busP, params, range.start );
    INIT_HEX( busP, params, range.end );
    INIT_STR( busP, params, linkName );
    INIT_INT( busP, params, clock);
    INIT_BOOL( busP, params, responder_set );
    INIT_INT( busP, params, block_size);
    INIT_INT( busP, params, bus_id );
    INIT_INT( busP, params, header_cycles);
    INIT_INT( busP, params, width);

    return new BusPlus( static_cast<M5*>(comp), &busP );
}

BusPlus::BusPlus( M5* comp, const BusPlusParams *p )  : 
    Bus( p ),
    m_comp( comp ),
    m_startAddr( p->range.start), 
    m_endAddr( p->range.end) 
{
    DBGX(2,"linkname `%s`\n", p->linkName.c_str() );

    m_link = comp->configureLink( p->linkName, "1ns",
        new SST::Event::Handler<BusPlus>( this, &BusPlus::eventHandler ) );

    if ( ! m_link ) {
        _error( BusPlus, "configureLink()\n");
    }

    m_tc = comp->registerTimeBase("1ns");
    if ( ! m_tc ) {
        _error( BusPlus, "registerTimeBase()\n");
    }
    m_link->setDefaultTimeBase(m_tc);

    m_port.resize(1);
    m_port[0].busPort = getPort( "port", 5 ); 
    m_port[0].linkPort = new LinkPort( p->name + ".linkPort", this );

    m_port[0].busPort->setPeer( m_port[0].linkPort ); 
    m_port[0].linkPort->setPeer( m_port[0].busPort ); 
}

void BusPlus::init() {
    m_port[0].linkPort->sendStatusChange( Port::RangeChange );
}  

void BusPlus::getAddressRanges(AddrRangeList &resp, bool &snoop ) {
    DPRINTFN("getAddressRanges:\n");
    snoop = false;
    resp.clear();
    resp.push_back( RangeSize(m_startAddr, m_endAddr));
}

bool BusPlus::send( MemEvent* event )
{
    // Note that we are not throttling data on this link
    m_link->Send( event );
    return true;
}

void BusPlus::eventHandler( SST::Event* e )
{
    MemEvent* event = static_cast<MemEvent*>(e);
    PacketPtr pkt = event->M5_Packet();
    Cycle_t now = m_tc->convertToCoreTime( m_comp->getCurrentSimTime(m_tc) );

    DBGX( 3,"SST-time=%lu `%s` %#lx\n", now,
        pkt->cmdString().c_str(), (long) pkt->getAddr() );

    DPRINTFN("eventHandler: `%s` %#lx\n", pkt->cmdString().c_str(),
                    (long) pkt->getAddr() );

    m_comp->catchup( now );

    switch ( event->type() ) {
        case MemEvent::Functional:
            m_port[0].linkPort->sendFunctional( pkt );
            break;

        case MemEvent::Timing:
            m_port[0].linkPort->sendTiming( pkt );
	    break;
    }

    m_comp->arm( now );

    delete event;
}

