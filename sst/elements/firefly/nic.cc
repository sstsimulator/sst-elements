// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/timeLord.h>
#include <sst/core/element.h>

#include <sstream>

#include "nic.h"

using namespace SST;
using namespace SST::Firefly;

static void print( Output& dbg, char* buf, int len );

Nic::Nic(ComponentId_t id, Params &params) :
    Component( id ),
    m_recvMachine( *this, m_dbg ),
    m_currentSend( NULL ),
    m_txDelay( 50 ),
    m_packetId(0),
    m_ftRadix(0),
    m_getKey(10)
{
    m_myNodeId = params.find_integer("nid", -1);
    assert( m_myNodeId != -1 );

    m_dbg.verbose(CALL_INFO,1,0,"\n");
    char buffer[100];
    snprintf(buffer,100,"@t:%d:Nic::@p():@l ",m_myNodeId);

    m_dbg.init(buffer, params.find_integer("verboseLevel",0),0,
       (Output::output_location_t)params.find_integer("debug", 0));

    int rxMatchDelay = params.find_integer( "rxMatchDelay_ns", 100 );
    m_txDelay =      params.find_integer( "txDelay_ns", 50 );

    UnitAlgebra xxx( params.find_string( "packetSize" ) );
    if ( xxx.hasUnits( "B" ) ) {
        m_packetSizeInBytes = xxx.getRoundedValue(); 
    } else if ( xxx.hasUnits( "b" ) ) {
        m_packetSizeInBytes = xxx.getRoundedValue() / 8; 
    } else {
        assert(0);
    }
	m_packetSizeInBits = m_packetSizeInBytes * 8;

	UnitAlgebra buf_size( params.find_string("buffer_size") );
	UnitAlgebra link_bw( params.find_string("link_bw") );

    m_dbg.verbose(CALL_INFO,1,0,"id=%d buffer_size=%s link_bw=%s "
			"packetSize=%d\n", m_myNodeId, buf_size.toString().c_str(),
			link_bw.toString().c_str(),m_packetSizeInBytes);

    m_linkControl = (Merlin::LinkControl*)loadSubComponent(
                    params.find_string("module"), this, params);
    assert( m_linkControl );

	m_linkControl->configure(params.find_string("rtrPortName","rtr"),
                             link_bw, 1, buf_size, buf_size);

    m_recvNotifyFunctor =
        new Merlin::LinkControl::Handler<Nic>(this,&Nic::recvNotify );
    assert( m_recvNotifyFunctor );

    m_sendNotifyFunctor =
        new Merlin::LinkControl::Handler<Nic>(this,&Nic::sendNotify );
    assert( m_sendNotifyFunctor );

    m_selfLink = configureSelfLink("Nic::selfLink", "1 ns",
        new Event::Handler<Nic>(this,&Nic::handleSelfEvent));
    assert( m_selfLink );

    m_linkControl->setNotifyOnReceive( m_recvNotifyFunctor );

    m_dbg.verbose(CALL_INFO,1,0,"%d\n", IdToNet( m_myNodeId ) );

    m_num_vNics = params.find_integer("num_vNics", 1 );
    for ( int i = 0; i < m_num_vNics; i++ ) {
        m_vNicV.push_back( new VirtNic( *this, i,
			params.find_string("corePortName","core") ) );
    }
    m_recvMachine.init( m_vNicV.size(), rxMatchDelay );
    m_memRgnM.resize( m_vNicV.size() );
}

Nic::~Nic()
{
	delete m_linkControl;

	if ( m_recvNotifyFunctor ) delete m_recvNotifyFunctor;
	if ( m_sendNotifyFunctor ) delete m_sendNotifyFunctor;

    for ( int i = 0; i < m_num_vNics; i++ ) {
        delete m_vNicV[i];
    }

#if 0
    // move to RecvMachine
    for ( unsigned i = 0; i < m_recvM.size(); i++ ) {
        std::map< int, std::deque<RecvEntry*> >::iterator iter;

        for ( iter = m_recvM[i].begin(); iter != m_recvM[i].end(); ++iter ) {
            while ( ! (*iter).second.empty() ) {
                delete (*iter).second.front();
                (*iter).second.pop_front(); 
            }
        }
    }

    while ( ! m_activeRecvM.empty() ) {
        delete m_activeRecvM.begin()->second;
        m_activeRecvM.erase( m_activeRecvM.begin() );
    }
#endif

    while ( ! m_sendQ.empty() ) {
        delete m_sendQ.front();
        m_sendQ.pop_front();
    }

    if ( m_currentSend ) delete m_currentSend;
}

Nic::VirtNic::VirtNic( Nic& nic, int _id, std::string portName ) : 
    m_nic( nic ),
    id( _id )
{
    std::ostringstream tmp;
    tmp <<  id;

    m_toCoreLink = nic.configureLink( portName + tmp.str(), "1 ns", 
        new Event::Handler<Nic::VirtNic>(
                    this, &Nic::VirtNic::handleCoreEvent ) );
	assert( m_toCoreLink );
}

void Nic::VirtNic::init( unsigned int phase )
{
    if ( 0 == phase ) {
        m_toCoreLink->sendInitData( new NicInitEvent( 
                m_nic.getNodeId(), id, m_nic.getNum_vNics() ) );
    }
}

void Nic::VirtNic::handleCoreEvent( Event* ev )
{
    m_nic.handleVnicEvent( ev, id );
}

void Nic::VirtNic::notifyRecvDmaDone( int src_vNic, int src, int tag,
										size_t len, void* key )
{
    m_toCoreLink->send(0, 
        new NicRespEvent( NicRespEvent::DmaRecv, src_vNic, src, tag, len, key ) );
}

void Nic::VirtNic::notifyNeedRecv( int src_vNic, int src, int tag, size_t len )
{
    m_toCoreLink->send(0, 
            new NicRespEvent( NicRespEvent::NeedRecv, src_vNic, src, tag, len ) );
}

void Nic::VirtNic::notifySendDmaDone( void* key )
{
    m_toCoreLink->send(0, new NicRespEvent( NicRespEvent::DmaSend, key ));
}

void Nic::VirtNic::notifySendPioDone( void* key )
{
    m_toCoreLink->send(0, new NicRespEvent( NicRespEvent::PioSend, key ));
}

void Nic::VirtNic::notifyGetDone( void* key )
{
    m_toCoreLink->send(0, new NicRespEvent( NicRespEvent::Get, key ));
}

void Nic::VirtNic::notifyPutDone( void* key )
{
    m_toCoreLink->send(0, new NicRespEvent( NicRespEvent::Put, key ));
}


void Nic::init( unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    if ( 0 == phase ) {
        for ( unsigned int i = 0; i < m_vNicV.size(); i++ ) {
            m_dbg.verbose(CALL_INFO,1,0,"sendInitdata to core %d\n", i );
            m_vNicV[i]->init( phase );
        } 
    } 
    m_linkControl->init(phase);
}

void Nic::handleVnicEvent( Event* ev, int id )
{
    NicCmdEvent* event = static_cast<NicCmdEvent*>(ev);

    m_dbg.verbose(CALL_INFO,3,0,"%d\n",event->type);

    switch ( event->type ) {
    case NicCmdEvent::DmaSend:
        dmaSend( event, id );
        break;
    case NicCmdEvent::DmaRecv:
        dmaRecv( event, id );
        break;
    case NicCmdEvent::PioSend:
        pioSend( event, id );
        break;
    case NicCmdEvent::Get:
        get( event, id );
        break;
    case NicCmdEvent::Put:
        put( event, id );
        break;
    case NicCmdEvent::RegMemRgn:
        regMemRgn( event, id );
        break;
    default:
        assert(0);
    }
}

int Nic::IdToNet( int v )
{
    return v;
}
int Nic::NetToId( int v )
{
    return v;
}

void Nic::handleSelfEvent( Event *e )
{
    SelfEvent* event = static_cast<SelfEvent*>(e);

    switch ( event->type ) {

      case SelfEvent::RunRecvMachine:
        m_recvMachine.run();
        break;

      case SelfEvent::RunSendMachine:
        if ( event->entry ) {
            m_sendQ.push_back( static_cast<SendEntry*>(event->entry) ); 
        }
        m_dbg.verbose(CALL_INFO,2,0,"ProcessSend\n");
        processSend();
        break;
    }

    if ( e ) {
        delete e;
    }
}

void Nic::schedSend( uint64_t delay ) {

    SelfEvent* event = new SelfEvent;
    event->type = SelfEvent::RunSendMachine;
    event->entry = NULL;
    schedEvent( event, delay );
}

void Nic::processSend()
{
    m_dbg.verbose(CALL_INFO,2,0,"number pending %lu\n", m_sendQ.size() );

    if ( m_currentSend ) {
        if ( m_linkControl->spaceToSend(0,m_packetSizeInBits) ) { 
            m_currentSend = processSend( m_currentSend );
        }
        if ( m_currentSend && !
                    m_linkControl->spaceToSend(0,m_packetSizeInBits) )
        { 
            m_dbg.verbose(CALL_INFO,2,0,"set send notify\n");
            m_linkControl->setNotifyOnSend( m_sendNotifyFunctor );
        }
    }

    if ( ! m_currentSend && ! m_sendQ.empty() ) {
        m_currentSend = m_sendQ.front();
        m_sendQ.pop_front();
        m_dbg.verbose(CALL_INFO,2,0,"new Send\n");
        schedSend( m_txDelay );
    }

    if ( ! m_currentSend ) {
        m_dbg.verbose(CALL_INFO,2,0,"remove send notify\n");
        m_linkControl->setNotifyOnSend( NULL );
    }
}

Nic::SendEntry* Nic::processSend( SendEntry* entry )
{
    bool ret = false;
    while ( m_linkControl->spaceToSend(0,m_packetSizeInBits) && entry )  
    {
        MerlinFireflyEvent* ev = new MerlinFireflyEvent;

        if ( 0 == entry->currentVec && 0 == entry->currentPos  ) {
            
            MsgHdr hdr;

            hdr.op= entry->getOp();
            hdr.tag = entry->tag();
            hdr.len = entry->totalBytes();
            hdr.dst_vNicId = entry->dst_vNic();
            hdr.src_vNicId = entry->local_vNic(); 

            m_dbg.verbose(CALL_INFO,1,0,"src_vNic=%d, send dstNid=%d "
                    "dst_vNic=%d tag=%#x bytes=%lu\n",
                    hdr.src_vNicId,entry->node(), 
                    hdr.dst_vNicId, hdr.tag, entry->totalBytes()) ;

            ev->buf.insert( ev->buf.size(), (const char*) &hdr, 
                                       sizeof(hdr) );
        }
        ret = copyOut( m_dbg, *ev, *entry ); 

        print(m_dbg, &ev->buf[0], ev->buf.size() );
        ev->setDest( IdToNet( entry->node() ) );
        ev->setSrc( IdToNet( m_myNodeId ) );
        ev->setPktSize();

        #if 0
        ev->setTraceType( Merlin::RtrEvent::FULL );
        ev->setTraceID( m_packetId++ );
        #endif
        m_dbg.verbose(CALL_INFO,2,0,"sending event with %lu bytes\n",
                                                        ev->buf.size());
        assert( ev->buf.size() );
        bool sent = m_linkControl->send( ev, 0 );
        assert( sent );

        if ( ret ) {

            m_dbg.verbose(CALL_INFO,1,0,"send entry done\n");

            entry->notify();

            delete entry;
            entry = NULL;
        }
    }
    return entry;
}

void Nic::dmaSend( NicCmdEvent *e, int vNicNum )
{
    SendEntry* entry = new SendEntry( vNicNum, e );
    m_dbg.verbose(CALL_INFO,1,0,"dest=%#x tag=%#x vecLen=%lu totalBytes=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );

    entry->setNotifier( new NotifyFunctor_2< Nic, int, void* >
                    ( this, &Nic::notifySendDmaDone, vNicNum, e->key) );
    
    m_sendQ.push_back( entry );

    schedSend();
}

void Nic::pioSend( NicCmdEvent *e, int vNicNum )
{
    SendEntry* entry = new SendEntry( vNicNum, e );
    m_dbg.verbose(CALL_INFO,1,0,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
        "vecLen=%lu totalBytes=%lu\n", vNicNum, e->node, e->dst_vNic,
                    e->tag, e->iovec.size(), entry->totalBytes() );

    entry->setNotifier( new NotifyFunctor_2< Nic, int, void* >
                    ( this, &Nic::notifySendPioDone, vNicNum, e->key) );

    m_sendQ.push_back( entry );

    schedSend();
}

void Nic::dmaRecv( NicCmdEvent *e, int vNicNum )
{
    RecvEntry* entry = new RecvEntry( vNicNum, e );

    m_dbg.verbose(CALL_INFO,1,0,"vNicNum=%d src=%d tag=%#x length=%lu\n",
                   vNicNum, e->node, e->tag, entry->totalBytes());

    m_recvMachine.addDma( entry->local_vNic(), e->tag, entry );
}


void Nic::get( NicCmdEvent *e, int vNicNum )
{
    int getKey = genGetKey();

    m_getOrgnM[ getKey ] = new PutRecvEntry( vNicNum, &e->iovec );

        m_dbg.verbose(CALL_INFO,2,0,"%p %lu\n",m_getOrgnM[getKey],
                            m_getOrgnM[ getKey ]->ioVec().size());

    m_getOrgnM[ getKey ]->setNotifier( new NotifyFunctor_2< Nic, int, void* >
            ( this, &Nic::notifyGetDone, vNicNum, e->key) );

    m_dbg.verbose(CALL_INFO,1,0,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
                        "vecLen=%lu totalBytes=%lu\n",
                vNicNum, e->node, e->dst_vNic, e->tag, e->iovec.size(), 
                m_getOrgnM[ getKey ]->totalBytes() );

    m_sendQ.push_back( new GetOrgnEntry( vNicNum, e, getKey) );

    schedSend();
}

void Nic::put( NicCmdEvent *e, int vNicNum )
{
    SendEntry* entry = new SendEntry( vNicNum, e );
    assert(0);
    m_dbg.verbose(CALL_INFO,1,0,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
                        "vecLen=%lu totalBytes=%lu\n",
                vNicNum, e->node, e->dst_vNic, e->tag, e->iovec.size(),
                entry->totalBytes() );

    entry->setNotifier( new NotifyFunctor_2< Nic, int, void* >
                    ( this, &Nic::notifyPutDone, vNicNum, e->key) );

    m_sendQ.push_back( entry );
    schedSend();
}

void Nic::regMemRgn( NicCmdEvent *e, int vNicNum )
{
    m_dbg.verbose(CALL_INFO,1,0,"rgnNum %d\n",e->tag);
    
    m_memRgnM[ vNicNum ][ e->tag ] = new MemRgnEntry( vNicNum, e->iovec );
    delete e;
}

// Merlin stuff
bool Nic::sendNotify(int)
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    
    schedSend();

    // remove notifier
    return false;
}

bool Nic::recvNotify(int vc)
{
    m_dbg.verbose(CALL_INFO,1,0,"network event available vc=%d\n",vc);
    assert( 0 == vc );

    m_recvMachine.run();

    // keep the current notifier because the recvMahcine may have changed it 
    return true;
}

void Nic::RecvMachine::run()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    bool blocked = false;
    do { 
        switch ( m_state ) {
          case NeedPkt:
            m_dbg.verbose(CALL_INFO,1,0,"NeedPkt\n");

            m_mEvent = getMerlinEvent(0);
            if ( ! m_mEvent ) {
                m_dbg.verbose(CALL_INFO,1,0,"no packet available\n");
                setNotify();
                blocked = true;
                break;
            }
            m_state = HavePkt;

          case HavePkt:
            m_dbg.verbose(CALL_INFO,1,0,"event has %lu bytes\n",
                                                    m_mEvent->buf.size() );

            if ( m_activeRecvM.find( m_mEvent->src ) == 
                                        m_activeRecvM.end() ) {
    
                SelfEvent* event = new SelfEvent;
                event->entry = NULL; 

                m_nic.schedEvent( event, 
                        processFirstEvent( *m_mEvent, m_state, event->entry ) );

                m_dbg.verbose(CALL_INFO,1,0,"state=%d\n",m_state);
                // if the packet was a "Get" it created a SendEntry,
                // schedule the send and continue receiving packets
                // else 
                // schedule a delay that runs the recvMachine
                if ( event->entry ) {
                    event->type = SelfEvent::RunSendMachine;
                    m_state = NeedPkt;
                } else {
                    event->type = SelfEvent::RunRecvMachine;
                    clearNotify();
                    blocked = true;
                } 
                break;
            }
            m_state = Move;

          case Move:
          case Put:
            m_dbg.verbose(CALL_INFO,1,0,"Move\n");
            assert( m_mEvent );
            moveEvent( m_mEvent );
            m_state = NeedPkt; 
            break;

          case WaitMove:
            assert(0);

          case NeedRecv: 
            m_dbg.verbose(CALL_INFO,1,0,"NeedRecv\n"); 
            processNeedRecv( m_mEvent );
            m_state = HavePkt; 
            blocked = true;
            break;
        }
        m_dbg.verbose(CALL_INFO,1,0,"state=%d %s\n",m_state,
                    blocked?"blocked":"not blocked");
    } while ( ! blocked );
}

uint64_t Nic::RecvMachine::processFirstEvent( MerlinFireflyEvent& mEvent,
                        RecvMachine::State& state, Entry*& sEntry  )
{
    int delay = 0;
        
    MsgHdr& hdr = *(MsgHdr*) &mEvent.buf[0];

    if ( MsgHdr::Msg == hdr.op ) {
        m_dbg.verbose(CALL_INFO,2,0,"Msg Op\n");

        delay = m_rxMatchDelay;

        if ( findRecv( mEvent.src, hdr ) ) {
            mEvent.buf.erase(0, sizeof(MsgHdr) );
            state = Move;
        } else {
            state = NeedRecv;
        }

    } else if ( MsgHdr::Rdma == hdr.op ) {

        RdmaMsgHdr& rdmaHdr = *(RdmaMsgHdr*)&mEvent.buf[ sizeof(MsgHdr) ];

        switch ( rdmaHdr.op  ) {

          case RdmaMsgHdr::Put:
          case RdmaMsgHdr::GetResp:
            m_dbg.verbose(CALL_INFO,2,0,"Put Op\n");
            state = Put;
            assert( findPut( mEvent.src, hdr, rdmaHdr ) );
            break;

          case RdmaMsgHdr::Get:
            m_dbg.verbose(CALL_INFO,2,0,"Get Op\n");
            sEntry = findGet( mEvent.src, hdr, rdmaHdr );
            delay = 200; // host read  delay
            break;

          default:
            assert(0);
        }

        mEvent.buf.erase(0, sizeof(MsgHdr) + sizeof(rdmaHdr) );
    }

    return delay;
}

bool Nic::RecvMachine::findPut( int src, MsgHdr& hdr, RdmaMsgHdr& rdmahdr )
{
    m_dbg.verbose(CALL_INFO,2,0,"src=%d len=%lu\n",src,hdr.len); 
    m_dbg.verbose(CALL_INFO,2,0,"rgnNum=%d offset=%d respKey=%d\n",
            rdmahdr.rgnNum, rdmahdr.offset, rdmahdr.respKey); 

    if ( RdmaMsgHdr::GetResp == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,0,"GetResp\n");
        m_activeRecvM[src] = m_nic.m_getOrgnM[ rdmahdr.respKey ];


        m_dbg.verbose(CALL_INFO,2,0,"%p %lu\n", m_activeRecvM[src],
                    m_activeRecvM[src]->ioVec().size());
        m_nic.m_getOrgnM.erase(rdmahdr.respKey);
        return true;

    } else if ( RdmaMsgHdr::Put == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,0,"Put\n");
        assert(0);
    } else {
        assert(0);
    }

    return false;
}

Nic::SendEntry* Nic::RecvMachine::findGet( int src, 
                        MsgHdr& hdr, RdmaMsgHdr& rdmaHdr  )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    // Note that we are not doing a strong check here. We are only checking that
    // the tag matches. 
    if ( m_nic.m_memRgnM[ hdr.dst_vNicId ].find( hdr.tag ) ==
                                m_nic.m_memRgnM[ hdr.dst_vNicId ].end() ) {
        assert(0);
    }

    assert( hdr.tag == rdmaHdr.rgnNum );
    
    m_dbg.verbose(CALL_INFO,1,0,"rgnNum %d offset=%d respKey=%d\n",
                        rdmaHdr.rgnNum, rdmaHdr.offset, rdmaHdr.respKey );

    MemRgnEntry* entry = m_nic.m_memRgnM[ hdr.dst_vNicId ][rdmaHdr.rgnNum]; 
    assert( entry );

    m_nic.m_memRgnM[ hdr.dst_vNicId ].erase(rdmaHdr.rgnNum); 

    return new PutOrgnEntry( entry->vNicNum(), 
                                    src, hdr.src_vNicId, 
                                    rdmaHdr.respKey, entry );
}


bool Nic::RecvMachine::findRecv( int src, MsgHdr& hdr )
{
    m_dbg.verbose(CALL_INFO,2,0,"need a recv entry, srcNic=%d src_vNic=%d "
                "dst_vNic=%d tag=%#x len=%lu\n", src, hdr.src_vNicId,
						hdr.dst_vNicId, hdr.tag, hdr.len);

    if ( m_recvM[hdr.dst_vNicId].find( hdr.tag ) == m_recvM[hdr.dst_vNicId].end() ) {
		m_dbg.verbose(CALL_INFO,2,0,"did't match tag\n");
        return false;
    }

    RecvEntry* entry = m_recvM[ hdr.dst_vNicId][ hdr.tag ].front();
    if ( entry->node() != -1 && entry->node() != src ) {
		m_dbg.verbose(CALL_INFO,2,0,"didn't match node  want=%#x src=%#x\n",
											entry->node(), src );
        return false;
    } 
    m_dbg.verbose(CALL_INFO,2,0,"recv entry size %lu\n",entry->totalBytes());

    if ( entry->totalBytes() < hdr.len ) {
        assert(0);
    }

    m_dbg.verbose(CALL_INFO,2,0,"found a receive entry\n");

    m_activeRecvM[src] = m_recvM[ hdr.dst_vNicId ][ hdr.tag ].front();
    m_recvM[ hdr.dst_vNicId ][ hdr.tag ].pop_front();

    if ( m_recvM[ hdr.dst_vNicId ][ hdr.tag ].empty() ) {
        m_recvM[ hdr.dst_vNicId ].erase( hdr.tag );
    }
    m_activeRecvM[src]->setMatchInfo( src, hdr );
    m_activeRecvM[src]->setNotifier( 
        new NotifyFunctor_6<Nic,int,int,int,int,size_t,void*>
                (&m_nic,&Nic::notifyRecvDmaDone,
                    m_activeRecvM[src]->local_vNic(),
                    m_activeRecvM[src]->src_vNic(),
                    m_activeRecvM[src]->match_src(),
                    m_activeRecvM[src]->match_tag(),
                    m_activeRecvM[src]->match_len(),
                    m_activeRecvM[src]->key() )
    );

    return true;
}

void Nic::RecvMachine::moveEvent( MerlinFireflyEvent* event )
{
    int src = event->src;
    m_dbg.verbose(CALL_INFO,1,0,"event has %lu bytes\n", event->buf.size() );

    if ( 0 == m_activeRecvM[ src ]->currentVec && 
             0 == m_activeRecvM[ src ]->currentPos  ) {
        m_dbg.verbose(CALL_INFO,1,0,"local_vNic %d, recv srcNic=%d "
                    "src_vNic=%d tag=%x bytes=%lu\n", 
                        m_activeRecvM[ src ]->local_vNic(),
                        src,
                        m_activeRecvM[ src ]->src_vNic(),
                        m_activeRecvM[ src ]->match_tag(),
                        m_activeRecvM[ src ]->match_len() );
    }
    long tmp = event->buf.size();
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    if ( copyIn( m_dbg, *m_activeRecvM[ src ], *event ) || 
        m_activeRecvM[src]->match_len() == 
                        m_activeRecvM[src]->currentLen ) {
        m_dbg.verbose(CALL_INFO,1,0,"recv entry done, srcNic=%d src_vNic=%d\n",
                            src, m_activeRecvM[ src ]->src_vNic() );

        m_dbg.verbose(CALL_INFO,2,0,"copyIn %lu bytes\n",
                                            tmp - event->buf.size());

        m_activeRecvM[src]->notify();

        delete m_activeRecvM[src];
        m_activeRecvM.erase(src);

        if ( ! event->buf.empty() ) {
            m_dbg.fatal(CALL_INFO,-1,
                "buffer not empty, %lu bytes remain\n",event->buf.size());
        }
    }

    m_dbg.verbose(CALL_INFO,2,0,"copyIn %lu bytes\n", tmp - event->buf.size());

    if ( event->buf.empty() ) {
        m_dbg.verbose(CALL_INFO,1,0,"network event is done\n");
        delete event;
    }
}

static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    dbg.verbose(CALL_INFO,4,0,"addr=%p len=%d\n",buf,len);
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,3,0,"%#03x\n",(unsigned char)buf[i]);
    }
}

size_t Nic::RecvMachine::copyIn( Output& dbg, Nic::Entry& entry,
                                        MerlinFireflyEvent& event )
{
    dbg.verbose(CALL_INFO,3,0,"ioVec.size()=%lu\n", entry.ioVec().size() );


    for ( ; entry.currentVec < entry.ioVec().size();
                entry.currentVec++, entry.currentPos = 0 ) {

        if ( entry.ioVec()[entry.currentVec].len ) {
            size_t toLen = entry.ioVec()[entry.currentVec].len - entry.currentPos;
            size_t fromLen = event.buf.size();
            size_t len = toLen < fromLen ? toLen : fromLen;

            char* toPtr = (char*) entry.ioVec()[entry.currentVec].ptr +
                                                        entry.currentPos;
            dbg.verbose(CALL_INFO,3,0,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            entry.currentLen += len;
            if ( entry.ioVec()[entry.currentVec].ptr ) {
                memcpy( toPtr, &event.buf[0], len );
                print( dbg, toPtr, len );
            }

            event.buf.erase(0,len);

            entry.currentPos += len;
            if ( 0 == event.buf.size() &&
                    entry.currentPos != entry.ioVec()[entry.currentVec].len )
            {
                dbg.verbose(CALL_INFO,3,0,"event buffer empty\n");
                break;
            }
        }
    }

    dbg.verbose(CALL_INFO,3,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec().size() ) ;
}

bool  Nic::copyOut( Output& dbg, MerlinFireflyEvent& event, Nic::Entry& entry )
{
    dbg.verbose(CALL_INFO,3,0,"ioVec.size()=%lu\n", entry.ioVec().size() );

    for ( ; entry.currentVec < entry.ioVec().size() &&
                event.buf.size() <  m_packetSizeInBytes;
                entry.currentVec++, entry.currentPos = 0 ) {

        dbg.verbose(CALL_INFO,3,0,"vec[%lu].len %lu\n",entry.currentVec,
                    entry.ioVec()[entry.currentVec].len );

        if ( entry.ioVec()[entry.currentVec].len ) {
            size_t toLen = m_packetSizeInBytes - event.buf.size();
            size_t fromLen = entry.ioVec()[entry.currentVec].len -
                                                        entry.currentPos;

            size_t len = toLen < fromLen ? toLen : fromLen;

            dbg.verbose(CALL_INFO,3,0,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            const char* from = 
                    (const char*) entry.ioVec()[entry.currentVec].ptr + 
                                                        entry.currentPos;

            if ( entry.ioVec()[entry.currentVec].ptr ) {
                event.buf.insert( event.buf.size(), from, len );
            } else {
                event.buf.insert( event.buf.size(), len, 0 );
            }

            entry.currentPos += len;
            if ( event.buf.size() == m_packetSizeInBytes &&
                    entry.currentPos != entry.ioVec()[entry.currentVec].len ) {
                break;
            }
        }
    }
    dbg.verbose(CALL_INFO,3,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec().size() ) ;
}

BOOST_CLASS_EXPORT( NicInitEvent )
