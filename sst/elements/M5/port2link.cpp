#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <port2link.h>
#include <m5.h>

#include <debug.h>
#include <paramHelp.h>

using namespace std;

BOOST_CLASS_EXPORT(MemEvent)

struct Port2LinkParams : public MemObjectParams
{
    std::string linkName;
    int clock;
    int block_size;
    int header_cycles;
    int width;
};

extern "C" {
    SimObject* create_Port2Link( void*, std::string name, Params& params );
}

SimObject* create_Port2Link( void* comp, std::string name, Params& params )
{
    Port2LinkParams& tmp = *new Port2LinkParams;
    tmp.name = name;

    INIT_STR( tmp, params, linkName );
    INIT_INT( tmp, params, clock );
    INIT_INT( tmp, params, block_size );
    INIT_INT( tmp, params, header_cycles );
    INIT_INT( tmp, params, width );

    return new Port2Link( static_cast<M5*>(comp), &tmp);
}

Port2Link::Port2Link( M5* comp, const Port2LinkParams *p ) :
    m_comp( comp ),
    MemObject(p),
    m_memObjPort( NULL),
    tickNextIdle( 0 ),
    clock(p->clock),
    defaultBlockSize(p->block_size),
    headerCycles( p->header_cycles ),
    width( p->width ),
    linkIdleEvent( this ),
    inRetry( false )
{
    DBGX(2,"linkname `%s`\n", p->linkName.c_str() );

    m_link = comp->configureLink( p->linkName, "1ns",
        new SST::Event::Handler<Port2Link>( this, &Port2Link::eventHandler ) );

    if ( ! m_link ) {
        _error( Port2Link, "configureLink()\n");
    }

    m_tc = comp->registerTimeBase("1ns");
    if ( ! m_tc ) {
        _error( Port2Link, "registerTimeBase()\n");
    }
    m_link->setDefaultTimeBase(m_tc);
}

void Port2Link::eventHandler( SST::Event* e )
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
            // TODO: need to check deferred pkt queue
            m_memObjPort->sendFunctional( pkt );
            break;

        case MemEvent::Timing:

            if ( pkt->memInhibitAsserted() ) {
               DPRINTFN( "`%s` %#lx memInhibitAsserted()\n",
                    pkt->cmdString().c_str(), (long) pkt->getAddr() );
                if ( pkt->isRead() )  {
                DPRINTFN( "`%s` %#lx memInhibitAsserted()\n",
                    pkt->cmdString().c_str(), (long) pkt->getAddr() );
                    //delete pkt;
                }
            } else {
                if ( ! m_deferred.empty() ) {
                    DBGX(3,"defer\n");
                    m_deferred.push_back( pkt );
                } else if ( m_memObjPort->sendTiming( pkt ) == false ) {
                    DBGX(3,"defer\n");
                    m_deferred.push_back( pkt );
                }
            }
            break;
    }

    m_comp->arm( now );

    delete e;
}

void Port2Link::init() {
    m_memObjPort->sendStatusChange( Port::RangeChange );
}

Port *Port2Link::getPort(const std::string &if_name, int idx )
{
    DBGX( 2, " getPort %s %d\n", if_name.c_str(), idx );
    DPRINTFN(" getPort %s %d\n", if_name.c_str(), idx );

    if ( if_name.compare( "memObj" ) == 0 ) {
        if ( m_memObjPort ) {
            panic("Port2Link::getPort: memObj port assigned", idx);
        }
        return m_memObjPort = new MemObjPort( name() + ".memObj", this );
    }

    return NULL;
}

bool Port2Link::recvTiming(PacketPtr pkt)
{ 
    if ( tickNextIdle > curTick || ( ! inRetry && ! m_deferredPkt.empty() ) ) 
    {
        m_deferredPkt.push_back(pkt);
        DPRINTFN("recvTiming: %s %#lx BUSY\n", pkt->cmdString().c_str(),
                    (long)pkt->getAddr());
        DBGX(3,"%s %#lx BUSY\n", pkt->cmdString().c_str(),
                    (long)pkt->getAddr());
        return false;
    }

    DBGX( 3,"SST-time=%lu `%s` %#lx\n", 
        m_tc->convertToCoreTime( m_comp->getCurrentSimTime(m_tc) ), 
        pkt->cmdString().c_str(), (long) pkt->getAddr() );

    DPRINTFN("recvTiming: %s %#lx\n", pkt->cmdString().c_str(), 
                                        (long)pkt->getAddr());

    calcPacketTiming(pkt);
    occupyLink( pkt->finishTime );

    // Packet was successfully sent.
    // Also take care of retries
    if (inRetry) {
        DPRINTFN("Remove retry from list\n");
        m_deferredPkt.pop_front();
        inRetry = false;
    }

    m_link->Send( new MemEvent( pkt ) );

    return true;
}

void Port2Link::recvFunctional(PacketPtr pkt ) 
{
    if (!pkt->isPrint()) {
        // don't do DPRINTFs on PrintReq as it clutters up the output
        DPRINTFN( "recvFunctional: addr %#lx cmd %s\n",
                (long) pkt->getAddr(), pkt->cmdString().c_str());
    }

    dynamic_cast<Packet::PrintReqState*>(pkt->senderState);

    if ( ! pkt->cmd.isWrite() ) {
        _error( Port2Link, "read is not supported\n" );
    }

    MemEvent* event = new MemEvent( pkt, MemEvent::Functional );
    m_link->Send(event);
}

void Port2Link::recvRetry( int id ) 
{
    DBGX(3,"%lu id=%d\n",curTick,id);
    if ( id == -1 ) {
        if ( ! m_deferredPkt.empty() && curTick >= tickNextIdle ) {
            inRetry = true;
            PacketPtr pkt = m_deferredPkt.front();

            DPRINTFN("Sending a retry to %s\n",
                        m_memObjPort->getPeer()->name().c_str());

            m_memObjPort->sendRetry();
            if ( inRetry ) {
                m_deferredPkt.pop_front();
                inRetry = false;

                while( tickNextIdle < curTick )
                    tickNextIdle += clock;
                reschedule(linkIdleEvent, tickNextIdle, true);
            }
        }
    } else {

        while ( ! m_deferred.empty() ) {
            if ( ! m_memObjPort->sendTiming( m_deferred.front() ) ) {
                break;
            }
            m_deferred.pop_front( );
        }
    }
}

void Port2Link::getAddressRanges(AddrRangeList &resp, bool &snoop ) {
    DPRINTFN("getAddressRanges:\n");
    snoop = false;
    resp.clear();
    int start = 0x100000;
    int end = 0x20000000 - start; 
    resp.push_back( RangeSize(start, end));
}

Tick Port2Link::calcPacketTiming(PacketPtr pkt)
{
    // Bring tickNextIdle up to the present tick.
    // There is some potential ambiguity where a cycle starts, which
    // might make a difference when devices are acting right around a
    // cycle boundary. Using a < allows things which happen exactly on
    // a cycle boundary to take up only the following cycle. Anything
    // that happens later will have to "wait" for the end of that
    // cycle, and then start using the bus after that.
    if (tickNextIdle < curTick) {
        tickNextIdle = curTick;
        if (tickNextIdle % clock != 0)
            tickNextIdle = curTick - (curTick % clock) + clock;
    }

    Tick headerTime = tickNextIdle + headerCycles * clock;

    // The packet will be sent. Figure out how long it occupies the bus, and
    // how much of that time is for the first "word", aka bus width.
    int numCycles = 0;
    if (pkt->hasData()) {
        // If a packet has data, it needs ceil(size/width) cycles to send it
        int dataSize = pkt->getSize();
        numCycles += dataSize/width;
        if (dataSize % width)
            numCycles++;
    }

    // The first word will be delivered after the current tick, the delivery
    // of the address if any, and one bus cycle to deliver the data
    pkt->firstWordTime = headerTime + clock;

    pkt->finishTime = headerTime + numCycles * clock;

    return headerTime;
}

void Port2Link::occupyLink(Tick until)
{
    if (until == 0) {
        // shortcut for express snoop packets
        return;
    }

    tickNextIdle = until;
    reschedule(linkIdleEvent, tickNextIdle, true);

    DPRINTFN("The link is now occupied from tick %lu to %lu\n",
                                        (long)curTick, (long)tickNextIdle);
}
