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
    m_sendMachine( *this, m_dbg ),
    m_recvMachine( *this, m_dbg ),
    m_packetId(0),
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
    int txDelay =      params.find_integer( "txDelay_ns", 50 );

    UnitAlgebra xxx( params.find_string( "packetSize" ) );
    int packetSizeInBytes;
    if ( xxx.hasUnits( "B" ) ) {
        packetSizeInBytes = xxx.getRoundedValue(); 
    } else if ( xxx.hasUnits( "b" ) ) {
        packetSizeInBytes = xxx.getRoundedValue() / 8; 
    } else {
        assert(0);
    }
	int packetSizeInBits = packetSizeInBytes * 8;

	UnitAlgebra buf_size( params.find_string("buffer_size") );
	UnitAlgebra link_bw( params.find_string("link_bw") );

    m_dbg.verbose(CALL_INFO,1,0,"id=%d buffer_size=%s link_bw=%s "
			"packetSize=%d\n", m_myNodeId, buf_size.toString().c_str(),
			link_bw.toString().c_str(), packetSizeInBytes);

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
    m_sendMachine.init( txDelay, packetSizeInBytes, packetSizeInBits );
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

void Nic::handleSelfEvent( Event *e )
{
    SelfEvent* event = static_cast<SelfEvent*>(e);

    switch ( event->type ) {

      case SelfEvent::RunRecvMachine:
        m_recvMachine.run();
        break;

      case SelfEvent::RunSendMachine:
        m_dbg.verbose(CALL_INFO,2,0,"ProcessSend\n");
        m_sendMachine.run( static_cast<SendEntry*>(event->entry) );
        break;
    }

    delete e;
}

void Nic::dmaSend( NicCmdEvent *e, int vNicNum )
{
    SendEntry* entry = new SendEntry( vNicNum, e );
    m_dbg.verbose(CALL_INFO,1,0,"dest=%#x tag=%#x vecLen=%lu totalBytes=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );

    entry->setNotifier( new NotifyFunctor_2< Nic, int, void* >
                    ( this, &Nic::notifySendDmaDone, vNicNum, e->key) );
    
    m_sendMachine.run( entry );
}

void Nic::pioSend( NicCmdEvent *e, int vNicNum )
{
    SendEntry* entry = new SendEntry( vNicNum, e );
    m_dbg.verbose(CALL_INFO,1,0,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
        "vecLen=%lu totalBytes=%lu\n", vNicNum, e->node, e->dst_vNic,
                    e->tag, e->iovec.size(), entry->totalBytes() );

    entry->setNotifier( new NotifyFunctor_2< Nic, int, void* >
                    ( this, &Nic::notifySendPioDone, vNicNum, e->key) );

    m_sendMachine.run( entry );
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

    m_sendMachine.run( new GetOrgnEntry( vNicNum, e, getKey) );
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

    m_sendMachine.run( entry );
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
    
    m_sendMachine.run();

    // keep the current notifier because the sendMahcine may have changed it 
    return true;
}

bool Nic::recvNotify(int vc)
{
    m_dbg.verbose(CALL_INFO,1,0,"network event available vc=%d\n",vc);
    assert( 0 == vc );

    m_recvMachine.run();

    // keep the current notifier because the recvMahcine may have changed it 
    return true;
}

BOOST_CLASS_EXPORT( NicInitEvent )
