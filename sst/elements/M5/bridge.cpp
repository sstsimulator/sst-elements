#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <bridge.h>

extern "C" {
    SimObject* create_Bridge( SST::Component*, std::string name,
                                                    SST::Params& params );
}

SimObject* create_Bridge( SST::Component* comp, std::string name,
                                                    SST::Params& params )
{
    BridgeParams& P = *new BridgeParams;

    P.name = name;

    P.range[0].start = params.find_integer( "range0.start" );
    P.range[0].end = params.find_integer( "range0.end" );
    P.mapTo[0] = params.find_integer( "range0.mapTo" );

    P.range[1].start = params.find_integer( "range1.start" );
    P.range[1].end = params.find_integer( "range1.end" );
    P.mapTo[1] = params.find_integer( "range1.mapTo" );

#if 0
    busP.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );
    busP.params = params.find_prefix_params( "link." );
#endif

    return new Bridge( &P );
}


Bridge::Bridge( const BridgeParams *params ) :
    MemObject( params ),
    m_range( params->range )
{
    m_offset[0] = params->mapTo[0] - m_range[0].start;
    m_offset[1] = params->mapTo[1] - m_range[1].start;
    m_ports[0] = m_ports[1] = NULL;

    DBGX(2,"%#lx-%#lx -> %#lx, offset=%#lx\n", m_range[0].start,
            m_range[0].end, params->mapTo[0], m_offset[0] );
    DBGX(2,"%#lx-%#lx -> %#lx, offset=%#lx\n", m_range[1].start,
            m_range[1].end, params->mapTo[1], m_offset[1] );
}

Port* Bridge::getPort( const std::string &if_name, int idx )
{
    DBGX(2,"name=`%s` idx=%d\n", if_name.c_str(), idx );
    assert ( ! ( idx < 0 || idx > 1 ) );
    assert ( ! m_ports[idx] );
    
    std::stringstream tmp;
    tmp << idx;
    m_ports[idx] = new BridgePort( name() + ".bridgePort." + tmp.str(), 
                    this, idx ); 
    return m_ports[idx];
}

void Bridge::init( void )
{
    DBGX(2,"\n" );
    m_ports[0]->sendStatusChange( Port::RangeChange );
    m_ports[1]->sendStatusChange( Port::RangeChange );
}

void Bridge::getAddressRanges(AddrRangeList &resp, bool &snoop, int idx ) {
    DBGX(2,"getAddressRanges: port %d\n",idx);
    snoop = false;
    resp.clear();
    resp.push_back( RangeSize(m_range[idx].start, m_range[idx].size()));
}

bool Bridge::recvTiming( PacketPtr pkt, int idx )
{
    if ( pkt->isRequest() ) {
        DBGX(2,"idx=%d Request addr=%#lx->%#lx\n",idx, pkt->getAddr(), 
                            pkt->getAddr() + m_offset[idx] );

        ::Request *req = pkt->req;
        Request* newReq = new Request( req->getPaddr() + m_offset[idx], 
                        req->getSize(), req->getFlags() );
        PacketPtr newPkt;
        
        if ( req->getPaddr() == pkt->getAddr() && 
                            req->getSize() == pkt->getSize() ) 
        {
            newPkt = new ::Packet( newReq, pkt->cmd, pkt->getDest() );
        } else {
            newPkt = new ::Packet( newReq, pkt->cmd, pkt->getDest(), 
                                                        pkt->getSize() );
        }
        newPkt->allocate();
        newPkt->setSrc( pkt->getSrc() );

        if ( pkt->needsResponse() ) {
            newPkt->senderState = static_cast<Packet::SenderState*>(
                            static_cast<void*>( pkt ) );
        } else {
            delete pkt;
        }
        pkt = newPkt;
    } else {
        PacketPtr orig = static_cast<PacketPtr>(
                            static_cast<void*>(pkt->senderState) ); 
        DBGX(2,"idx=%d Response addr=%#lx->%#lx\n",idx, 
                                pkt->getAddr(), orig->getAddr() ); 
        if ( ! orig->memInhibitAsserted() && pkt->hasData() ) {
            orig->setData( pkt->getPtr<uint8_t>() ); 
            orig->makeResponse();
        }
        delete pkt;
        pkt = orig;
    } 

    m_ports[ (idx + 1) % 2 ]->sendTiming(pkt);
}
