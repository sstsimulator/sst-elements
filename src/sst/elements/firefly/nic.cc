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
using namespace std::placeholders;

Nic::Nic(ComponentId_t id, Params &params) :
    Component( id ),
    m_sendNotify( 2, false ),
    m_sendNotifyCnt(0),
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
    m_recvM.resize( m_num_vNics );

    m_shmem = new Shmem( *this, m_dbg );


    m_recvMachine.push_back( new RecvMachine( *this, 0, m_vNicV.size(), m_myNodeId, 
                params.find<uint32_t>("verboseLevel",0),
                params.find<uint32_t>("verboseMask",-1), 
                rxMatchDelay, hostReadDelay, 
                std::bind( &Shmem::findRegion, m_shmem, _1 ) ) );
    m_recvMachine.push_back( new CtlMsgRecvMachine( *this, 1, m_vNicV.size(), m_myNodeId, 
                params.find<uint32_t>("verboseLevel",0),
                params.find<uint32_t>("verboseMask",-1), 
                rxMatchDelay, hostReadDelay, 
                std::bind( &Shmem::findRegion, m_shmem, _1 ) ) );

    m_sendMachine.push_back( new SendMachine( *this,  m_myNodeId, 
                params.find<uint32_t>("verboseLevel",0),
                params.find<uint32_t>("verboseMask",-1), 
                txDelay, packetSizeInBytes, 0  ) );
    m_sendMachine.push_back( new SendMachine( *this,  m_myNodeId,
                params.find<uint32_t>("verboseLevel",0),
                params.find<uint32_t>("verboseMask",-1), 
                txDelay, packetSizeInBytes, 1  ) );
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
    for ( unsigned i = 0; i < m_recvM.size(); i++ ) {
        std::map< int, std::deque<DmaRecvEntry*> >::iterator iter;

        for ( iter = m_recvM[i].begin(); iter != m_recvM[i].end(); ++iter ) {
            while ( ! (*iter).second.empty() ) {
                delete (*iter).second.front();
                (*iter).second.pop_front();
            }
        }
    }
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
    m_sendMachine[0]->printStatus( out );
    m_sendMachine[1]->printStatus( out );
    m_recvMachine[0]->printStatus( out );
    m_recvMachine[1]->printStatus( out );
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
    NicCmdBaseEvent* event = static_cast<NicCmdBaseEvent*>(ev);

    m_dbg.verbose(CALL_INFO,3,1,"type=%d\n",event->base_type);
    
    switch ( event->base_type ) {
    case NicCmdBaseEvent::Msg:
        handleMsgEvent( static_cast<NicCmdEvent*>(event), id );
        break;
    case NicCmdBaseEvent::Shmem:
        handleShmemEvent( static_cast<NicShmemCmdEvent*>(event), id );
        break;
    default:
        assert(0);
    }
}

void Nic::handleMsgEvent( NicCmdEvent* event, int id )
{
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

void Nic::handleShmemEvent( NicShmemCmdEvent* event, int id )
{

    switch (event->type) {
    case NicShmemCmdEvent::PutP:
    case NicShmemCmdEvent::PutV:
        m_shmem->put( event, id );
        break;
    case NicShmemCmdEvent::GetP:
    case NicShmemCmdEvent::GetV:
        m_shmem->get( event, id );
        break;
    case NicShmemCmdEvent::Fence:
        m_shmem->fence( event, id );
        break;
    case NicShmemCmdEvent::RegMem:
        m_shmem->regMem( event, id );
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
        m_sendMachine[0]->run( static_cast<SendEntryBase*>(event->entry) );
    }

    delete e;
}

void Nic::dmaSend( NicCmdEvent *e, int vNicNum )
{
    std::function<void(void*)> callback = std::bind( &Nic::notifySendPioDone, this, vNicNum, _1 );

    CmdSendEntry* entry = new CmdSendEntry( vNicNum, e, callback );

    m_dbg.verbose(CALL_INFO,1,1,"dest=%#x tag=%#x vecLen=%lu totalBytes=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );

    m_sendMachine[0]->run( entry );
}

void Nic::pioSend( NicCmdEvent *e, int vNicNum )
{
    std::function<void(void*)> callback = std::bind( &Nic::notifySendPioDone, this, vNicNum, _1 );

    CmdSendEntry* entry = new CmdSendEntry( vNicNum, e, callback );

    m_dbg.verbose(CALL_INFO,1,1,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
        "vecLen=%lu totalBytes=%lu\n", vNicNum, e->node, e->dst_vNic,
                    e->tag, e->iovec.size(), entry->totalBytes() );

    m_sendMachine[0]->run( entry );
}

void Nic::dmaRecv( NicCmdEvent *e, int vNicNum )
{
    DmaRecvEntry::Callback callback = std::bind( &Nic::notifyRecvDmaDone, this, vNicNum, _1, _2, _3, _4, _5 );
    
    DmaRecvEntry* entry = new DmaRecvEntry( e, callback );

    m_dbg.verbose(CALL_INFO,1,1,"vNicNum=%d src=%d tag=%#x length=%lu\n",
                   vNicNum, e->node, e->tag, entry->totalBytes());

    m_recvM[ vNicNum ][ e->tag ].push_back( entry );
    m_recvMachine[0]->addDma( );
}

void Nic::get( NicCmdEvent *e, int vNicNum )
{
    int getKey = genGetKey();

    DmaRecvEntry::Callback callback = std::bind( &Nic::notifyRecvDmaDone, this, vNicNum, _1, _2, _3, _4, _5 );

    m_getOrgnM[ getKey ] = new DmaRecvEntry( e, callback );

    m_dbg.verbose(CALL_INFO,1,1,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
                        "vecLen=%lu totalBytes=%lu\n",
                vNicNum, e->node, e->dst_vNic, e->tag, e->iovec.size(), 
                m_getOrgnM[ getKey ]->totalBytes() );

    m_sendMachine[1]->run( new GetOrgnEntry( vNicNum, e->node, e->dst_vNic, e->tag, getKey) );
}

void Nic::put( NicCmdEvent *e, int vNicNum )
{
    assert(0);

    std::function<void(void*)> callback = std::bind(  &Nic::notifyPutDone, this, vNicNum, _1 );
    CmdSendEntry* entry = new CmdSendEntry( vNicNum, e, callback );
    m_dbg.verbose(CALL_INFO,1,1,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
                        "vecLen=%lu totalBytes=%lu\n",
                vNicNum, e->node, e->dst_vNic, e->tag, e->iovec.size(),
                entry->totalBytes() );

    m_sendMachine[0]->run( entry );
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
        m_sendMachine[vc]->notify();
        m_sendNotify[vc] = false;
        --m_sendNotifyCnt;
    }

    // false equal remove notifier
    return m_sendNotifyCnt;
}

bool Nic::recvNotify(int vc)
{
    m_dbg.verbose(CALL_INFO,2,1,"network event available vc=%d\n",vc);

    m_recvMachine[vc]->notify( );

    // remove this notifier
    return false;
}

void Nic::detailedMemOp( Thornhill::DetailedCompute* detailed,
        std::vector<DmaVec>& vec, std::string op, Callback callback ) {

    std::deque< std::pair< std::string, SST::Params> > gens;
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_DETAILED_MEM,
                        "%s %zu vectors\n", op.c_str(), vec.size());

    for ( unsigned i = 0; i < vec.size(); i++ ) {

        Params params;
        std::stringstream tmp;

        if ( 0 == vec[i].addr ) {
            m_dbg.fatal(CALL_INFO,-1,"Invalid addr %" PRIx64 "\n", vec[i].addr);
        }
        if ( vec[i].addr < 0x100 ) {
            m_dbg.output(CALL_INFO,"Warn addr %" PRIx64 " ignored\n", vec[i].addr);
            i++;
            continue;
        }

        if ( 0 == vec[i].length ) {
            i++;
            m_dbg.verbose(CALL_INFO,-1,NIC_DBG_DETAILED_MEM,
                    "skip 0 length vector addr=0x%" PRIx64 "\n",vec[i].addr);
            continue;
        }

        int opWidth = 8;
        m_dbg.verbose(CALL_INFO,1,NIC_DBG_DETAILED_MEM,
                "addr=0x%" PRIx64 " length=%zu\n",
                vec[i].addr,vec[i].length);
        size_t count = vec[i].length / opWidth;
        count += vec[i].length % opWidth ? 1 : 0;
        tmp << count;
        params.insert( "count", tmp.str() );

        tmp.str( std::string() ); tmp.clear();
        tmp << opWidth;
        params.insert( "length", tmp.str() );

        tmp.str( std::string() ); tmp.clear();
        tmp << vec[i].addr;
        params.insert( "startat", tmp.str() );

        tmp.str( std::string() ); tmp.clear();
        tmp << 0x100000000;
        params.insert( "max_address", tmp.str() );

        params.insert( "memOp", op );
        #if  INSERT_VERBOSE
        params.insert( "generatorParams.verbose", "1" );
        params.insert( "verbose", "5" );
        #endif

        gens.push_back( std::make_pair( "miranda.SingleStreamGenerator", params ) );
    }

    if ( gens.empty() ) {
        schedCallback( callback, 0 );
    } else {
        std::function<int()> foo = [=](){
            callback( );
            return 0;
        };
        detailed->start( gens, foo );
    }
}

void Nic::dmaRead( std::vector<DmaVec>& vec, Callback callback ) {

    if ( m_useDetailedCompute && m_detailedCompute[0] ) {

        detailedMemOp( m_detailedCompute[0], vec, "Read", callback );

    } else {
        size_t len = 0;
        for ( unsigned i = 0; i < vec.size(); i++ ) {
            len += vec[i].length;
        }
        m_arbitrateDMA->canIRead( callback, len );
    }
}

void Nic::dmaWrite( std::vector<DmaVec>& vec, Callback callback ) {

    if ( m_useDetailedCompute && m_detailedCompute[1] ) {

        detailedMemOp( m_detailedCompute[1], vec, "Write", callback );

    } else {
        size_t len = 0;
        for ( unsigned i = 0; i < vec.size(); i++ ) {
            len += vec[i].length;
        }
        m_arbitrateDMA->canIWrite( callback, len );
    }
}

Nic::DmaRecvEntry* Nic::findPut( int src, MsgHdr& hdr, RdmaMsgHdr& rdmahdr )
{
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,
                    "src=%d len=%lu\n",src,hdr.len);
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,
                    "rgnNum=%d offset=%d respKey=%d\n",
            rdmahdr.rgnNum, rdmahdr.offset, rdmahdr.respKey);

    DmaRecvEntry* entry = NULL;
    if ( RdmaMsgHdr::GetResp == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"GetResp\n");
        entry = m_getOrgnM[ rdmahdr.respKey ];

        m_getOrgnM.erase(rdmahdr.respKey);

    } else if ( RdmaMsgHdr::Put == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Put\n");
        assert(0);
    } else {
        assert(0);
    }

    return entry;
}

Nic::SendEntryBase* Nic::findGet( int src,
                        MsgHdr& hdr, RdmaMsgHdr& rdmaHdr  )
{
    // Note that we are not doing a strong check here. We are only checking that
    // the tag matches.
    if ( m_memRgnM[ hdr.dst_vNicId ].find( rdmaHdr.rgnNum ) ==
                                m_memRgnM[ hdr.dst_vNicId ].end() ) {
        assert(0);
    }

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"rgnNum=%d offset=%d respKey=%d\n",
                        rdmaHdr.rgnNum, rdmaHdr.offset, rdmaHdr.respKey );

    MemRgnEntry* entry = m_memRgnM[ hdr.dst_vNicId ][rdmaHdr.rgnNum];
    assert( entry );

    m_memRgnM[ hdr.dst_vNicId ].erase(rdmaHdr.rgnNum);

    return new PutOrgnEntry( entry->vNicNum(),
                                    src, hdr.src_vNicId,
                                    rdmaHdr.respKey, entry );
}

Nic::EntryBase* Nic::findRecv( int srcNode, MsgHdr& hdr, int tag  )
{
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"need a recv entry, srcNic=%d src_vNic=%d "
                "dst_vNic=%d tag=%#x len=%lu\n", srcNode, hdr.src_vNicId,
                        hdr.dst_vNicId, tag, hdr.len);

    if ( m_recvM[hdr.dst_vNicId].find( tag ) == m_recvM[hdr.dst_vNicId].end() ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"did't match tag\n");
        return NULL;
    }

    DmaRecvEntry* entry = m_recvM[ hdr.dst_vNicId][ tag ].front();
    if ( entry->node() != -1 && entry->node() != srcNode ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,
                "didn't match node  want=%#x src=%#x\n",
                                            entry->node(), srcNode );
        return NULL;
    }
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,
                "recv entry size %lu\n",entry->totalBytes());

    if ( entry->totalBytes() < hdr.len ) {
        assert(0);
    }

    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"found a receive entry\n");

    m_recvM[ hdr.dst_vNicId ][ tag ].pop_front();
    if ( m_recvM[ hdr.dst_vNicId ][ tag ].empty() ) {
        m_recvM[ hdr.dst_vNicId ].erase( tag );
    }
    return entry;
}

std::pair<Hermes::MemAddr, size_t> Nic::findShmem( uint64_t simVAddr )
{
    return m_shmem->findRegion( simVAddr);
}
