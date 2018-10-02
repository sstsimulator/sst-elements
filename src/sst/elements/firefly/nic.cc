// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
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

int Nic::MaxPayload = (int)((1L<<32) - 1);
int Nic::m_packetId = 0;
int Nic::ShmemSendMove::m_alignment = 64;

Nic::Nic(ComponentId_t id, Params &params) :
    Component( id ),
    m_detailedCompute( 2, NULL ),
    m_useDetailedCompute(false),
    m_getKey(10),
    m_memoryModel(NULL),
    m_nic2host_base_lat_ns(0),
    m_respKey(1),
    m_curNetworkSrc(-1),
	m_predNetIdleTime(0)
{
    m_myNodeId = params.find<int>("nid", -1);
    assert( m_myNodeId != -1 );

    char buffer[100];
    snprintf(buffer,100,"@t:%d:Nic::@p():@l ",m_myNodeId);

    m_dbg.init(buffer, 
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",-1), 
        Output::STDOUT);

	// The link between the NIC and HOST historically provided the latency of crossing a bus such as PCI
	// hence it was configured at wire up with a value like 150ns. The NIC has since taken on some HOST functionality
	// so the latency has been dropped to 1ns. The bus latency must still be added for some messages so the 
	// NIC now sends these message to itself with a time of nic2host_lat_ns - "the latency of the link". 
	m_nic2host_lat_ns = calcDelay_ns( params.find<SST::UnitAlgebra>("nic2host_lat", SST::UnitAlgebra("150ns")));
	if ( m_nic2host_lat_ns > m_nic2host_base_lat_ns ) { 
		m_nic2host_base_lat_ns = 1;
	}

    int rxMatchDelay = params.find<int>( "rxMatchDelay_ns", 100 );
    m_txDelay =      params.find<int>( "txDelay_ns", 50 );
    int hostReadDelay = params.find<int>( "hostReadDelay_ns", 200 );
    m_shmemRxDelay_ns = params.find<int>( "shmemRxDelay_ns",0); 

    m_num_vNics = params.find<int>("num_vNics", 1 );

	for ( unsigned i = 0; i < m_num_vNics; i++  ) {
		m_sendStreamNum.push_back(0); 
	}

    m_tracedNode =     params.find<int>( "tracedNode", -1 );
    m_tracedPkt  =     params.find<int>( "tracedPkt", -1 );
    int numShmemCmdSlots =    params.find<int>( "numShmemCmdSlots", 32 );
    int maxSendMachineQsize = params.find<int>( "maxSendMachineQsize", 1 );
    int maxRecvMachineQsize = params.find<int>( "maxRecvMachineQsize", 1 );
    Nic::ShmemSendMove::m_alignment = params.find<int>("shmemSendAlignment",64);
    int numSendMachines = params.find<int>( "numSendMachines",1);
    if ( numSendMachines < 1 ) {
        m_dbg.fatal(CALL_INFO,-1,"Error: numSendMachines must be greater than 1, requested %d\n",numSendMachines);
    }    
    ++numSendMachines;

    int numRecvNicUnits = params.find<int>( "numRecvNicUnits", 1 );
    m_unitPool = new UnitPool( 
        m_dbg,
        params.find<std::string>( "nicAllocationPolicy", "RoundRobin" ), 
        numRecvNicUnits,
        numSendMachines,
        1,
        m_num_vNics
    );

    int packetOverhead = params.find<int>("packetOverhead",0);
    UnitAlgebra xxx = params.find<SST::UnitAlgebra>( "packetSize" );
    int packetSizeInBytes;
    if ( xxx.hasUnits( "B" ) ) {
        packetSizeInBytes = xxx.getRoundedValue(); 
    } else if ( xxx.hasUnits( "b" ) ) {
        packetSizeInBytes = xxx.getRoundedValue() / 8; 
    } else {
        assert(0);
    }

    int minPktPayload = 32;
    assert( ( packetSizeInBytes - packetOverhead ) >= minPktPayload );

	UnitAlgebra input_buf_size = params.find<SST::UnitAlgebra>("input_buf_size" );
	UnitAlgebra output_buf_size = params.find<SST::UnitAlgebra>("output_buf_size" );
	UnitAlgebra link_bw = params.find<SST::UnitAlgebra>("link_bw" );

	m_linkBytesPerSec = link_bw.getRoundedValue()/8;

    m_dbg.verbose(CALL_INFO,1,1,"id=%d input_buf_size=%s output_buf_size=%s link_bw=%s "
			"packetSize=%d\n", m_myNodeId, 
            input_buf_size.toString().c_str(),
            output_buf_size.toString().c_str(),
			link_bw.toString().c_str(), packetSizeInBytes);

    m_linkControl = (SimpleNetwork*)loadSubComponent(
                    params.find<std::string>("module"), this, params);
    assert( m_linkControl );

	m_linkControl->initialize(params.find<std::string>("rtrPortName","rtr"),
                              link_bw, 2, input_buf_size, output_buf_size);

    m_recvNotifyFunctor =
        new SimpleNetwork::Handler<Nic>(this,&Nic::recvNotify );
    assert( m_recvNotifyFunctor );

    m_linkRecvWidget = new LinkControlWidget( m_dbg, 
        [=]() {
            m_dbg.debug(CALL_INFO,2,1,"call setNotifyOnReceive\n");
            m_linkControl->setNotifyOnReceive( m_recvNotifyFunctor );
        } 
    );

    m_sendNotifyFunctor =
        new SimpleNetwork::Handler<Nic>(this,&Nic::sendNotify );
    assert( m_sendNotifyFunctor );

    m_linkSendWidget = new LinkControlWidget( m_dbg,
        [=]() {
            m_dbg.debug(CALL_INFO,2,1,"call setNotifyOnSend\n");
            m_linkControl->setNotifyOnSend( m_sendNotifyFunctor );
        }
    );

    m_selfLink = configureSelfLink("Nic::selfLink", "1 ns",
        new Event::Handler<Nic>(this,&Nic::handleSelfEvent));
    assert( m_selfLink );

    m_dbg.verbose(CALL_INFO,2,1,"IdToNet()=%d\n", IdToNet( m_myNodeId ) );

    for ( int i = 0; i < m_num_vNics; i++ ) {
        m_vNicV.push_back( new VirtNic( *this, i,
			params.find<std::string>("corePortName","core") ) );
    }

	Params shmemParams = params.find_prefix_params( "shmem." ); 
    m_shmem = new Shmem( *this, shmemParams, m_myNodeId, m_num_vNics, m_dbg, numShmemCmdSlots, getDelay_ns(), getDelay_ns() );

    if ( params.find<int>( "useSimpleMemoryModel", 0 ) ) {
        Params smmParams = params.find_prefix_params( "simpleMemoryModel." );
        smmParams.insert( "busLatency",  std::to_string(m_nic2host_lat_ns), false );

		std::string useCache = smmParams.find<std::string>("useHostCache","all");
		std::string useBus = smmParams.find<std::string>("useBusBridge","all");

		if ( findNid( m_myNodeId, useCache ) ) {
        	smmParams.insert( "useHostCache",  "1", true );
		} else {
        	smmParams.insert( "useHostCache",  "0", true );
		}
		if ( findNid( m_myNodeId, useBus ) ) {
        	smmParams.insert( "useBusBridge",  "1", true );
		} else {
        	smmParams.insert( "useBusBridge",  "0", true );
		}
        
        std::stringstream tmp;
		tmp << m_myNodeId;
		smmParams.insert( "id", tmp.str(), true );


        tmp.str( std::string() ); tmp.clear();

		tmp << m_num_vNics;
		smmParams.insert( "numCores", tmp.str(), true );
        tmp.str( std::string() ); tmp.clear();

		tmp << m_unitPool->getTotal();
		smmParams.insert( "numNicUnits", tmp.str(), true );

        m_memoryModel = dynamic_cast<MemoryModel*>(loadSubComponent( "firefly.SimpleMemory",this, smmParams ));
    }
    if ( params.find<int>( "useTrivialMemoryModel", 0 ) ) {
		if ( m_memoryModel ) {
			m_dbg.fatal(CALL_INFO,0,"can't used TrivialMemoryModel, memoryModel already configured\n" );
		}
        Params smmParams = params.find_prefix_params( "simpleMemoryModel." );
    	m_memoryModel = new TrivialMemoryModel( this, smmParams );
    }
    
    m_recvMachine = new RecvMachine( *this, 0, m_vNicV.size(), m_myNodeId, 
                params.find<uint32_t>("verboseLevel",0),
                params.find<uint32_t>("verboseMask",-1), 
                rxMatchDelay, hostReadDelay, maxRecvMachineQsize, 
                params.find<>( "maxActiveRecvStreams", 1024*1024) );

    m_sendMachineV.resize(numSendMachines);
    for ( int i = 0; i < numSendMachines - 1; i++ ) {
        SendMachine* sm = new SendMachine( *this,  m_myNodeId, 
                params.find<uint32_t>("verboseLevel",0),
                params.find<uint32_t>("verboseMask",-1), 
                i, packetSizeInBytes, packetOverhead, maxSendMachineQsize, allocNicSendUnit(), false );
        m_sendMachineQ.push_back( sm  ); 
        m_sendMachineV[i] = sm;
    }
    
    m_sendMachineV[ numSendMachines - 1] = new SendMachine( *this,  m_myNodeId, 
                params.find<uint32_t>("verboseLevel",0),
                params.find<uint32_t>("verboseMask",-1), 
                m_sendMachineV.size()-1, packetSizeInBytes, packetOverhead, maxSendMachineQsize, allocNicSendUnit(), true ); 

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

	m_sentByteCount =     registerStatistic<uint64_t>("sentByteCount");
	m_rcvdByteCount =     registerStatistic<uint64_t>("rcvdByteCount");
	m_sentPkts = 	      registerStatistic<uint64_t>("sentPkts");
	m_rcvdPkts =          registerStatistic<uint64_t>("rcvdPkts");
	m_networkStall =      registerStatistic<uint64_t>("networkStall");
	m_hostStall =         registerStatistic<uint64_t>("hostStall");

	m_recvStreamPending = registerStatistic<uint64_t>("recvStreamPending");
	m_sendStreamPending = registerStatistic<uint64_t>("sendStreamPending");

    Statistic<uint64_t>* m_sentByteCount;
    Statistic<uint64_t>* m_rcvdByteCount;
    Statistic<uint64_t>* m_sentPkts;
    Statistic<uint64_t>* m_rcvdPkts;
}

Nic::~Nic()
{
    if ( m_memoryModel ) {
        delete m_memoryModel;
    }  
	delete m_shmem;
	delete m_linkControl;

    int numRcvd = m_recvMachine->getNumReceived();
    int numSent=0;
	delete m_recvMachine;
    for ( int i = 0; i <  m_sendMachineV.size(); i++ ) {
        numSent += m_sendMachineV[i]->getNumSent();
		delete m_sendMachineV[i];
	}
    m_dbg.debug(CALL_INFO,1,1,"                                                             finish numSent=%d numRcvd=%d\n",numSent,numRcvd);

	if ( m_recvNotifyFunctor ) delete m_recvNotifyFunctor;
	if ( m_sendNotifyFunctor ) delete m_sendNotifyFunctor;

	if ( m_detailedCompute[0] ) delete m_detailedCompute[0];
	if ( m_detailedCompute[1] ) delete m_detailedCompute[1];

    for ( int i = 0; i < m_num_vNics; i++ ) {
        delete m_vNicV[i];
    }
	delete m_arbitrateDMA;
}

void Nic::init( unsigned int phase )
{
    m_dbg.debug(CALL_INFO,1,1,"phase=%d\n",phase);
    if ( 0 == phase ) {
        for ( unsigned int i = 0; i < m_vNicV.size(); i++ ) {
            m_dbg.debug(CALL_INFO,1,1,"sendInitdata to core %d\n", i );
            m_vNicV[i]->init( phase );
        } 
    } 
    m_linkControl->init(phase);
}

void Nic::handleVnicEvent( Event* ev, int id )
{
    NicCmdBaseEvent* event = static_cast<NicCmdBaseEvent*>(ev);

    switch ( event->base_type ) {

      case NicCmdBaseEvent::Msg:
		m_selfLink->send( getDelay_ns( ), new SelfEvent( ev, id ) );
        break;

      case NicCmdBaseEvent::Shmem:
		m_shmem->handleEvent( static_cast<NicShmemCmdEvent*>(event), id );
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

void Nic::handleSelfEvent( Event *e )
{
    SelfEvent* event = static_cast<SelfEvent*>(e);
    
	switch ( event->type ) {
	case SelfEvent::Callback:
        event->callback();
		break;
	case SelfEvent::Event:
		handleVnicEvent2( event->event, event->linkNum );
		break;
	}
    delete e;
}

void Nic::handleVnicEvent2( Event* ev, int id )
{
    NicCmdBaseEvent* event = static_cast<NicCmdBaseEvent*>(ev);

    m_dbg.debug(CALL_INFO,3,1,"core=%d type=%d\n",id,event->base_type);

    switch ( event->base_type ) {
    case NicCmdBaseEvent::Msg:
        handleMsgEvent( static_cast<NicCmdEvent*>(event), id );
        break;
    case NicCmdBaseEvent::Shmem:
        m_shmem->handleNicEvent2( static_cast<NicShmemCmdEvent*>(event), id );
        break;
    default:
        assert(0);
    }
}

void Nic::dmaSend( NicCmdEvent *e, int vNicNum )
{
    std::function<void(void*)> callback = std::bind( &Nic::notifySendPioDone, this, vNicNum, _1 );

    CmdSendEntry* entry = new CmdSendEntry( vNicNum, getSendStreamNum(vNicNum), e, callback );

    m_dbg.debug(CALL_INFO,1,1,"dest=%#x tag=%#x vecLen=%lu totalBytes=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );

    qSendEntry( entry );
}

void Nic::pioSend( NicCmdEvent *e, int vNicNum )
{
    std::function<void(void*)> callback = std::bind( &Nic::notifySendPioDone, this, vNicNum, _1 );

    CmdSendEntry* entry = new CmdSendEntry( vNicNum, getSendStreamNum(vNicNum), e, callback );

    m_dbg.debug(CALL_INFO,1,1,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
        "vecLen=%lu totalBytes=%lu\n", vNicNum, e->node, e->dst_vNic,
                    e->tag, e->iovec.size(), entry->totalBytes() );

    qSendEntry( entry );
}

void Nic::dmaRecv( NicCmdEvent *e, int vNicNum )
{
    DmaRecvEntry::Callback callback = std::bind( &Nic::notifyRecvDmaDone, this, vNicNum, _1, _2, _3, _4, _5 );
    
    DmaRecvEntry* entry = new DmaRecvEntry( e, callback );

    m_dbg.debug(CALL_INFO,1,1,"vNicNum=%d src=%d tag=%#x length=%lu\n",
                   vNicNum, e->node, e->tag, entry->totalBytes());

    	
    m_recvMachine->postRecv( vNicNum, entry );
}

void Nic::get( NicCmdEvent *e, int vNicNum )
{
    int getKey = genGetKey();

    DmaRecvEntry::Callback callback = std::bind( &Nic::notifyRecvDmaDone, this, vNicNum, _1, _2, _3, _4, _5 );

    DmaRecvEntry* entry = new DmaRecvEntry( e, callback );
    m_recvMachine->regGetOrigin( vNicNum, getKey, entry);

    m_dbg.debug(CALL_INFO,1,1,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x vecLen=%lu totalBytes=%lu\n",
                vNicNum, e->node, e->dst_vNic, e->tag, e->iovec.size(), entry->totalBytes() );

    qSendEntry( new GetOrgnEntry( vNicNum, getSendStreamNum(vNicNum), e->node, e->dst_vNic, e->tag, getKey) );
}

void Nic::put( NicCmdEvent *e, int vNicNum )
{
    assert(0);

    std::function<void(void*)> callback = std::bind(  &Nic::notifyPutDone, this, vNicNum, _1 );
    CmdSendEntry* entry = new CmdSendEntry( vNicNum, getSendStreamNum(vNicNum), e, callback );
    m_dbg.debug(CALL_INFO,1,1,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x "
                        "vecLen=%lu totalBytes=%lu\n",
                vNicNum, e->node, e->dst_vNic, e->tag, e->iovec.size(),
                entry->totalBytes() );

    qSendEntry( entry );
}

void Nic::regMemRgn( NicCmdEvent *e, int vNicNum )
{
    m_dbg.debug(CALL_INFO,1,1,"rgnNum %d\n",e->tag);
    
    m_recvMachine->regMemRgn( vNicNum, e->tag, new MemRgnEntry( e->iovec ) );

    delete e;
}

void Nic::qSendEntry( SendEntryBase* entry ) {

    m_dbg.debug(CALL_INFO,2,NIC_DBG_SEND_MACHINE, "myPid=%d destNode=%d destPid=%d size=%zu %s\n",
                    entry->local_vNic(), entry->dest(), entry->dst_vNic(), entry->totalBytes(),
                    entry->isCtrl() ? "Ctrl" : entry->isAck() ? "Ack" : "Std");

    if ( entry->totalBytes() >= MaxPayload ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: maximum network message size must be less than %d bytes, requested size %zu\n",
                                        MaxPayload, entry->totalBytes());
    }
    if ( entry->isCtrl() || entry->isAck() ) {
        entry->setTxDelay(0);
        m_sendMachineV[m_sendMachineV.size() - 1]->qSendEntry( entry );
    } else {
        entry->setTxDelay(m_txDelay);

        int pid = entry->local_vNic();
        if ( ! m_sendMachineQ.empty() ) {
            assert( m_sendEntryQ.empty() );
            m_sendMachineQ.front()->run( entry );
            m_sendMachineQ.pop_front();         
        } else {
            m_sendEntryQ.push_back(std::make_pair( getCurrentSimTimeNano(), entry ) );
        }        
    }
}

void Nic::notifySendDone( SendMachine* mach, SendEntryBase* entry  ) {

    int pid = entry->local_vNic();
    m_dbg.debug(CALL_INFO,2,NIC_DBG_SEND_MACHINE,"machine=%d pid=%d\n", mach->getId(), pid );

    if ( m_sendEntryQ.empty() ) {
        m_sendMachineQ.push_back(mach);
    } else {
        mach->run(m_sendEntryQ.front().second );
        m_sendEntryQ.pop_front();
    }
}

void Nic::feedTheNetwork( )
{
    m_dbg.debug(CALL_INFO,5,NIC_DBG_SEND_NETWORK,"\n");

    int vc = 0;

    assert( m_curNetworkSrc >=0  );
    bool sentPkt;
    do {
        sentPkt = false;
        for ( unsigned i = 0; i < m_sendMachineV.size(); i++ ) {

            m_dbg.debug(CALL_INFO,4,NIC_DBG_SEND_NETWORK,"checking network src %d\n", m_curNetworkSrc);
            if ( ! m_sendMachineV[m_curNetworkSrc]->netPktQ_empty() ) {

                std::pair< FireflyNetworkEvent*, int>& pkt = m_sendMachineV[m_curNetworkSrc]->netPktQ_front();
                bool ret = m_linkControl->spaceToSend( vc, pkt.first->calcPayloadSizeInBits() );
                if ( ! ret ) {

                    m_dbg.debug(CALL_INFO,1,NIC_DBG_SEND_NETWORK,"blocking on network\n" );
                    schedCallback(
                        [=](){
                            m_linkSendWidget->setNotify( [=]() {
								SimTime_t curTime = Simulation::getSimulation()->getCurrentSimCycle();
								if ( curTime > m_predNetIdleTime ) {
									m_dbg.debug(CALL_INFO,1,NIC_DBG_SEND_NETWORK,"network stalled latency=%lld\n",
										curTime -  m_predNetIdleTime);
									m_networkStall->addData( curTime - m_predNetIdleTime );
								}
								feedTheNetwork();
							}, vc);
                        }
                    ,0 );

                    return;
                } else {

					SimTime_t curTime = Simulation::getSimulation()->getCurrentSimCycle();
					SimTime_t latPS = ( (double) pkt.first->payloadSize() / (double) m_linkBytesPerSec ) * 1000000000000; 

					if ( curTime > m_predNetIdleTime ) {
						m_predNetIdleTime = curTime;
					}
					m_predNetIdleTime += latPS;

                   	m_dbg.debug(CALL_INFO,1,NIC_DBG_SEND_NETWORK,"predNetIdleTime=%lld\n",m_predNetIdleTime );

                    sendPkt( pkt, vc );

                    m_sendMachineV[m_curNetworkSrc]->netPktQ_pop();
                    sentPkt = true;
                }
            }
            ++m_curNetworkSrc;
            m_curNetworkSrc %= m_sendMachineV.size();
        }
    } while( sentPkt );
    m_curNetworkSrc = -1;
}

void Nic::sendPkt( std::pair< FireflyNetworkEvent*, int>& entry, int vc )
{
    FireflyNetworkEvent* ev = entry.first;
    assert( ev->bufSize() );

    m_sentPkts->addData(1);


    SimpleNetwork::Request* req = new SimpleNetwork::Request();
    req->dest = IdToNet( entry.second );
    req->src = IdToNet( m_myNodeId );
    req->size_in_bits = ev->calcPayloadSizeInBits();
    req->vn = 0;
    req->givePayload( ev );

    if ( (m_tracedPkt == m_packetId || m_tracedPkt == -2) && m_tracedNode == getNodeId() )
    {
        req->setTraceType( SimpleNetwork::Request::ROUTE );
        req->setTraceID( m_packetId );
        ++m_packetId;
    }
    m_dbg.debug(CALL_INFO,3,NIC_DBG_SEND_NETWORK,
                    "dst=%" PRIu64 " sending event with %zu bytes packetId=%" PRIu64 " %s %s\n",req->dest,
                                                    ev->bufSize(), (uint64_t)m_packetId, 
                                                    ev->isHdr() ? "Hdr":"", 
                                                    ev->isTail() ? "Tail":"" );

	m_sentByteCount->addData( ev->payloadSize() );

    bool sent = m_linkControl->send( req, vc );
    assert( sent );
}

void Nic::detailedMemOp( Thornhill::DetailedCompute* detailed,
        std::vector<MemOp>& vec, std::string op, Callback callback ) {

    std::deque< std::pair< std::string, SST::Params> > gens;
    m_dbg.debug(CALL_INFO,1,NIC_DBG_DETAILED_MEM,
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
            m_dbg.debug(CALL_INFO,-1,NIC_DBG_DETAILED_MEM,
                    "skip 0 length vector addr=0x%" PRIx64 "\n",vec[i].addr);
            continue;
        }

        int opWidth = 8;
        m_dbg.debug(CALL_INFO,1,NIC_DBG_DETAILED_MEM,
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

void Nic::dmaRead( int unit, int pid, std::vector<MemOp>* vec, Callback callback ) {

    if ( m_memoryModel ) {
        calcNicMemDelay( unit, pid, vec, callback );
    } else {
		for ( unsigned i = 0;  i <  vec->size(); i++ ) {
			assert( (*vec)[i].callback == NULL );
		}
		if ( m_useDetailedCompute && m_detailedCompute[0] ) {

        	detailedMemOp( m_detailedCompute[0], *vec, "Read", callback );
        	delete vec;

    	} else {
        	size_t len = 0;
        	for ( unsigned i = 0; i < vec->size(); i++ ) {
            	len += (*vec)[i].length;
        	}
        	m_arbitrateDMA->canIRead( callback, len );
        	delete vec;
		}
    }
}

void Nic::dmaWrite( int unit, int pid, std::vector<MemOp>* vec, Callback callback ) {

    if ( m_memoryModel ) {
       	calcNicMemDelay( unit, pid, vec, callback );
    } else { 
		for ( unsigned i = 0;  i <  vec->size(); i++ ) {
			assert( (*vec)[i].callback == NULL );
		}
		if ( m_useDetailedCompute && m_detailedCompute[1] ) {

        	detailedMemOp( m_detailedCompute[1], *vec, "Write", callback );
        	delete vec;

    	} else {
        	size_t len = 0;
        	for ( unsigned i = 0; i < vec->size(); i++ ) {
            	len += (*vec)[i].length;
        	}
        	m_arbitrateDMA->canIWrite( callback, len );
        	delete vec;
    	}
	}
}

Hermes::MemAddr Nic::findShmem(  int core, Hermes::Vaddr addr, size_t length )
{
    std::pair<Hermes::MemAddr, size_t> region = m_shmem->findRegion( core, addr);

    m_dbg.debug(CALL_INFO,1,NIC_DBG_SHMEM,"found region core=%d Vaddr=%#" PRIx64 " length=%lu\n",
        core, region.first.getSimVAddr(), region.second );

    uint64_t offset =  addr - region.first.getSimVAddr();

    assert(  addr + length <= region.first.getSimVAddr() + region.second );
    return region.first.offset(offset);
}

