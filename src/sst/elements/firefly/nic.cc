// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
using namespace SST::Interfaces;

Nic::Nic(ComponentId_t id, Params &params) :
    Component( id ),
    m_sendNotify( 2, false ),
    m_sendNotifyCnt(0),
    m_sendMachine( 2, SendMachine( *this, m_dbg ) ),
    m_recvMachine( *this, m_dbg ),
    m_detailedCompute( 2, NULL ),
    m_useDetailedCompute(false),
    m_getKey(10)
{
    m_myNodeId = params.find<int>("nid", -1);
    assert( m_myNodeId != -1 );

    char buffer[100];
    snprintf(buffer,100,"@t:%d:Nic::@p():@l ",m_myNodeId);

    m_dbg.init(buffer, 
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",-1), 
        Output::STDOUT);

    int rxMatchDelay = params.find<int>( "rxMatchDelay_ns", 100 );
    int txDelay =      params.find<int>( "txDelay_ns", 50 );
    int hostReadDelay = params.find<int>( "hostReadDelay_ns", 200 );

    m_tracedNode =     params.find<int>( "tracedNode", -1 );
    m_tracedPkt  =     params.find<int>( "tracedPkt", -1 );

    UnitAlgebra xxx = params.find<SST::UnitAlgebra>( "packetSize" );
    int packetSizeInBytes;
    if ( xxx.hasUnits( "B" ) ) {
        packetSizeInBytes = xxx.getRoundedValue(); 
    } else if ( xxx.hasUnits( "b" ) ) {
        packetSizeInBytes = xxx.getRoundedValue() / 8; 
    } else {
        assert(0);
    }

	UnitAlgebra buf_size = params.find<SST::UnitAlgebra>("buffer_size" );
	UnitAlgebra link_bw = params.find<SST::UnitAlgebra>("link_bw" );

    m_dbg.verbose(CALL_INFO,1,1,"id=%d buffer_size=%s link_bw=%s "
			"packetSize=%d\n", m_myNodeId, buf_size.toString().c_str(),
			link_bw.toString().c_str(), packetSizeInBytes);

    m_linkControl = (SimpleNetwork*)loadSubComponent(
                    params.find<std::string>("module"), this, params);
    assert( m_linkControl );

	m_linkControl->initialize(params.find<std::string>("rtrPortName","rtr"),
                              link_bw, 2, buf_size, buf_size);

    m_recvNotifyFunctor =
        new SimpleNetwork::Handler<Nic>(this,&Nic::recvNotify );
    assert( m_recvNotifyFunctor );

    m_recvMachine.setNotify();

    m_sendNotifyFunctor =
        new SimpleNetwork::Handler<Nic>(this,&Nic::sendNotify );
    assert( m_sendNotifyFunctor );

    m_selfLink = configureSelfLink("Nic::selfLink", "1 ns",
        new Event::Handler<Nic>(this,&Nic::handleSelfEvent));
    assert( m_selfLink );

    m_dbg.verbose(CALL_INFO,2,1,"IdToNet()=%d\n", IdToNet( m_myNodeId ) );

    m_num_vNics = params.find<int>("num_vNics", 1 );
    for ( int i = 0; i < m_num_vNics; i++ ) {
        m_vNicV.push_back( new VirtNic( *this, i,
			params.find<std::string>("corePortName","core") ) );
    }
    m_recvMachine.init( m_vNicV.size(), rxMatchDelay, hostReadDelay );
    m_sendMachine[0].init( txDelay, packetSizeInBytes, 0 );
    m_sendMachine[1].init( txDelay, packetSizeInBytes, 1 );
    m_memRgnM.resize( m_vNicV.size() );

    float dmaBW  = params.find<float>( "dmaBW_GBs", 0.0 ); 
    float dmaContentionMult = params.find<float>( "dmaContentionMult", 0.0 );
    m_arbitrateDMA = new ArbitrateDMA( *this, m_dbg, dmaBW,
                                    dmaContentionMult, 100000 );

    Params dtldParams = params.find_prefix_params( "detailedCompute." );
    std::string dtldName =  dtldParams.find<std::string>( "name" );

    if ( ! dtldName.empty() ) {


        dtldParams.insert( "portName", "read", true );
        Thornhill::DetailedCompute* detailed;
        detailed = dynamic_cast<Thornhill::DetailedCompute*>( loadSubComponent(
                            dtldName, this, dtldParams ) );

        assert( detailed );
        if ( ! detailed->isConnected() ) {
            delete detailed;
        } else {
            m_detailedCompute[0] = detailed;
        }

        dtldParams.insert( "portName", "write", true );
        detailed = dynamic_cast<Thornhill::DetailedCompute*>( loadSubComponent(
                            dtldName, this, dtldParams ) );

        assert( detailed );
        if ( ! detailed->isConnected() ) {
            delete detailed;
        } else {
            m_detailedCompute[1] = detailed;
        }

	    m_useDetailedCompute = params.find<bool>("useDetailed", false );
    }
}

Nic::~Nic()
{
	delete m_linkControl;

	if ( m_recvNotifyFunctor ) delete m_recvNotifyFunctor;
	if ( m_sendNotifyFunctor ) delete m_sendNotifyFunctor;

    for ( int i = 0; i < m_num_vNics; i++ ) {
        delete m_vNicV[i];
    }
	delete m_arbitrateDMA;
}

void Nic::printStatus(Output &out)
{
    m_sendMachine[0].printStatus( out );
    m_sendMachine[1].printStatus( out );
    m_recvMachine.printStatus( out );
}

void Nic::init( unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,1,1,"phase=%d\n",phase);
    if ( 0 == phase ) {
        for ( unsigned int i = 0; i < m_vNicV.size(); i++ ) {
            m_dbg.verbose(CALL_INFO,1,1,"sendInitdata to core %d\n", i );
            m_vNicV[i]->init( phase );
        } 
    } 
    m_linkControl->init(phase);
}

void Nic::handleVnicEvent( Event* ev, int id )
{
    NicCmdEvent* event = static_cast<NicCmdEvent*>(ev);

    m_dbg.verbose(CALL_INFO,3,1,"%d\n",event->type);

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
    
    if ( event->callback ) {
        event->callback();
    } else if ( event->entry ) {
        m_sendMachine[0].run( static_cast<SendEntry*>(event->entry) );
    }

    delete e;
}

void Nic::dmaSend( NicCmdEvent *e, int vNicNum )
{
    SendEntry* entry = new SendEntry( vNicNum, e );
    m_dbg.verbose(CALL_INFO,1,1,"dest=%#x tag=%#x vecLen=%lu totalBytes=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );

    entry->setNotifier( new NotifyFunctor_2< Nic, int, void* >
                    ( this, &Nic::notifySendDmaDone, vNicNum, e->key) );
    
    m_sendMachine[0].run( entry );
}

void Nic::pioSend( NicCmdEvent *e, int vNicNum )
{
    SendEntry* entry = new SendEntry( vNicNum, e );
    m_dbg.verbose(CALL_INFO,1,1,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
        "vecLen=%lu totalBytes=%lu\n", vNicNum, e->node, e->dst_vNic,
                    e->tag, e->iovec.size(), entry->totalBytes() );

    entry->setNotifier( new NotifyFunctor_2< Nic, int, void* >
                    ( this, &Nic::notifySendPioDone, vNicNum, e->key) );

    m_sendMachine[0].run( entry );
}

void Nic::dmaRecv( NicCmdEvent *e, int vNicNum )
{
    RecvEntry* entry = new RecvEntry( vNicNum, e );

    m_dbg.verbose(CALL_INFO,1,1,"vNicNum=%d src=%d tag=%#x length=%lu\n",
                   vNicNum, e->node, e->tag, entry->totalBytes());

    m_recvMachine.addDma( entry->local_vNic(), e->tag, entry );
}


void Nic::get( NicCmdEvent *e, int vNicNum )
{
    int getKey = genGetKey();

    m_getOrgnM[ getKey ] = new PutRecvEntry( vNicNum, &e->iovec );

        m_dbg.verbose(CALL_INFO,2,1,"%p %lu\n",m_getOrgnM[getKey],
                            m_getOrgnM[ getKey ]->ioVec().size());

    m_getOrgnM[ getKey ]->setNotifier( new NotifyFunctor_2< Nic, int, void* >
            ( this, &Nic::notifyGetDone, vNicNum, e->key) );

    m_dbg.verbose(CALL_INFO,1,1,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
                        "vecLen=%lu totalBytes=%lu\n",
                vNicNum, e->node, e->dst_vNic, e->tag, e->iovec.size(), 
                m_getOrgnM[ getKey ]->totalBytes() );

    m_sendMachine[1].run( new GetOrgnEntry( vNicNum, e, getKey) );
}

void Nic::put( NicCmdEvent *e, int vNicNum )
{
    SendEntry* entry = new SendEntry( vNicNum, e );
    assert(0);
    m_dbg.verbose(CALL_INFO,1,1,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
                        "vecLen=%lu totalBytes=%lu\n",
                vNicNum, e->node, e->dst_vNic, e->tag, e->iovec.size(),
                entry->totalBytes() );

    entry->setNotifier( new NotifyFunctor_2< Nic, int, void* >
                    ( this, &Nic::notifyPutDone, vNicNum, e->key) );

    m_sendMachine[0].run( entry );
}

void Nic::regMemRgn( NicCmdEvent *e, int vNicNum )
{
    m_dbg.verbose(CALL_INFO,1,1,"rgnNum %d\n",e->tag);
    
    m_memRgnM[ vNicNum ][ e->tag ] = new MemRgnEntry( vNicNum, e->iovec );
    delete e;
}

// Merlin stuff
bool Nic::sendNotify(int vc)
{
    m_dbg.verbose(CALL_INFO,2,1,"network can send on vc=%d\n",vc);

    assert ( m_sendNotifyCnt > 0 );

    if ( m_sendNotify[vc] ) {
        m_sendMachine[vc].notify();
        m_sendNotify[vc] = false;
        --m_sendNotifyCnt;
    }

    // false equal remove notifier
    return m_sendNotifyCnt;
}

bool Nic::recvNotify(int vc)
{
    m_dbg.verbose(CALL_INFO,2,1,"network event available vc=%d\n",vc);

    m_recvMachine.notify( vc );

    // remove this notifier
    return false;
}

