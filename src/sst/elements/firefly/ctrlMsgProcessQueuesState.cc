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

#include <sst_config.h>

#include "ctrlMsgProcessQueuesState.h"
#include "ctrlMsgMemory.h"

using namespace SST::Firefly::CtrlMsg;
using namespace SST::Firefly;
using namespace SST;


ProcessQueuesState::ProcessQueuesState( Component* owner, Params& params ) : 
        SubComponent( owner ),

        m_getKey( 0 ),
        m_rspKey( 0 ),
        m_needRecv( 0 ),
        m_numNicRequestedShortBuff(0),
        m_numRecvLooped(0),
        m_missedInt( false ),
        m_intCtx(NULL)
{
    int level = params.find<uint32_t>("verboseLevel",0);
    int mask = params.find<int32_t>("verboseMask",-1);

    m_dbg.init("", level, mask, Output::STDOUT );

    m_statPstdRcv = registerStatistic<uint64_t>("posted_receive_list");
    m_statRcvdMsg = registerStatistic<uint64_t>("received_msg_list");

    m_msgTiming = new MsgTiming( parent, params, m_dbg );

    std::stringstream ss;
    ss << this;

    m_delayLink = configureSelfLink(
                        "ProcessQueuesStateSelfLink." + ss.str(), "1 ns",
                                new Event::Handler<ProcessQueuesState>(this,&ProcessQueuesState::delayHandler));

    m_loopLink = configureLink(
            params.find<std::string>("loopBackPortName", "loop"), "1 ns",
            new Event::Handler<ProcessQueuesState>(this,&ProcessQueuesState::loopHandler) );
    assert(m_loopLink);
}


ProcessQueuesState::~ProcessQueuesState()
{
    while ( ! m_postedShortBuffers.empty() ) {
        delete m_postedShortBuffers.begin()->first;
        delete m_postedShortBuffers.begin()->second;
        m_postedShortBuffers.erase( m_postedShortBuffers.begin() );
    }
}

void ProcessQueuesState::setVars( VirtNic* nic, Info* info, MemoryBase* mem, 
            Thornhill::MemoryHeapLink* memHeapLink, Link* returnToCaller )
{
    m_nic = nic;
    m_info = info;
    m_mem = mem;
    m_returnToCaller = returnToCaller;
    m_memHeapLink = memHeapLink;

    char buffer[100];
    snprintf(buffer,100,"@t:%#x:%d:CtrlMsg::ProcessQueuesState::@p():@l ",
                            m_nic->getNodeId(), m_info->worldRank());
    dbg().setPrefix(buffer);
}

void ProcessQueuesState:: finish() {
    dbg().verbose(CALL_INFO,1,0,"pstdRcvQ=%lu recvdMsgQ=%lu loopResp=%lu %lu\n",
    m_pstdRcvQ.size(), m_recvdMsgQ.size(), m_loopResp.size(), m_funcStack.size() );
}

void ProcessQueuesState::enterInit( bool haveGlobalMemHeap )
{
    size_t length = 0;
    length += MaxPostedShortBuffers * (sizeof(MatchHdr) + 16 ) & ~15;
    length += MaxPostedShortBuffers * (shortMsgLength() + 16 ) & ~15;
    length += MaxPostedShortBuffers * (sizeof(MatchHdr) + 16 ) & ~15;
    length += MaxPostedShortBuffers * (shortMsgLength() + 16 ) & ~15;

    std::function<void(uint64_t)> callback = [=](uint64_t value){
		enterInit_1( value, length );      
        return 0;
    };

    if ( haveGlobalMemHeap ) {
        memHeapAlloc( length, callback );
    } else {
        enterInit_1( 0x1000, length );
    }
}

void ProcessQueuesState::enterInit_1( uint64_t addr, size_t length )
{
    dbg().verbose(CALL_INFO,1,1,"simVAddr %" PRIx64 "  length=%zu\n", addr, length );

    m_simVAddrs = new HeapAddrs( addr, length );

    for ( unsigned long i = 0; i < MinPostedShortBuffers; i++ ) {
        postShortRecvBuffer();
    }

    passCtrlToFunction( );
}

void ProcessQueuesState::enterSend( _CommReq* req, uint64_t exitDelay )
{
    m_exitDelay = exitDelay;
    req->setSrcRank( getMyRank( req ) );
    dbg().verbose(CALL_INFO,1,1,"req=%p$ delay=%" PRIu64 "\n", req, exitDelay );

    uint64_t delay = txDelay( req->getLength() );

    VoidFunction callback;
    if ( m_nic->isLocal( calcNid( req, req->getDestRank() ) ) ) {
        callback =  std::bind( 
                    &ProcessQueuesState::processSendLoop, this, req );
    } else {
        callback = std::bind( 
                    &ProcessQueuesState::processSend_0, this, req );
    }
    schedCallback( callback, delay);
}

void ProcessQueuesState::processSend_0( _CommReq* req )
{
    m_mem->write( 
        std::bind( &ProcessQueuesState::processSend_1, this, req ),
        0, sizeof( req->hdr()) 
    ); 
}

void ProcessQueuesState::processSend_1( _CommReq* req )
{
    VoidFunction callback = std::bind( 
        &ProcessQueuesState::processSend_2, this, req );

    size_t length = req->getLength( );

    if ( length > shortMsgLength() ) {
        m_mem->pin( callback, 0, length );
    } else {
        m_mem->copy( callback, 0, 1, length );
    }
}

void ProcessQueuesState::processSend_2( _CommReq* req )
{
    void* hdrPtr = NULL;
    size_t length = req->getLength( );

    IoVec hdrVec;   
    hdrVec.len = sizeof( req->hdr() ); 

    hdrVec.addr.setSimVAddr( m_simVAddrs->alloc( hdrVec.len ) );

    if ( length <= shortMsgLength() ) {
		hdrVec.addr.setBacking( malloc( hdrVec.len ) );
        hdrPtr = hdrVec.addr.getBacking();
        memcpy( hdrVec.addr.getBacking(), &req->hdr(), hdrVec.len );
    } else {
        hdrVec.addr.setBacking( &req->hdr() ); 
    }

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );
    
    nid_t nid = calcNid( req, req->getDestRank() );

    if ( length <= shortMsgLength() ) {
        dbg().verbose(CALL_INFO,1,1,"Short %lu bytes dest %#x\n",length,nid); 
        vec.insert( vec.begin() + 1, req->ioVec().begin(), 
                                        req->ioVec().end() );
        req->setDone( sendReqFiniDelay( length ) );

    } else {
        dbg().verbose(CALL_INFO,1,1,"sending long message %lu bytes\n",length); 
        req->hdr().key = genGetKey();

        GetInfo* info = new GetInfo;


        info->req = req;

        std::vector<IoVec> hdrVec;
        hdrVec.resize(1);
        hdrVec[0].len = sizeof( info->hdr ); 
        hdrVec[0].addr.setSimVAddr( m_simVAddrs->alloc( hdrVec[0].len ) ); 
        hdrVec[0].addr.setBacking( &info->hdr ); 
        
        Callback2* callback = new Callback2;
        *callback = std::bind( 
            &ProcessQueuesState::dmaRecvFiniGI, this, info, hdrVec[0].addr.getSimVAddr(), 
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3
        );

        m_nic->dmaRecv( nid, req->hdr().key, hdrVec, callback ); 
        m_nic->regMem( nid, req->hdr().key, req->ioVec(), NULL );
    }

    VoidFunction* callback = new VoidFunction;
    *callback = std::bind( &ProcessQueuesState::pioSendFiniVoid, this, hdrPtr, hdrVec.addr.getSimVAddr() );

    m_nic->pioSend( nid, ShortMsgQ, vec, callback);

    if ( ! req->isBlocking() ) {
        exit();
    } else {
        enterWait( new WaitReq( req ), m_exitDelay );
    }
}

void ProcessQueuesState::processSendLoop( _CommReq* req )
{
    dbg().verbose(CALL_INFO,2,2,"key=%p\n", req);

    IoVec hdrVec;
    hdrVec.len = sizeof( req->hdr() );
    hdrVec.addr.setSimVAddr( 1 ); //m_simVAddrs->alloc( hdrVec.len );
    hdrVec.addr.setBacking( &req->hdr() );

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );
    vec.insert( vec.begin() + 1, req->ioVec().begin(), 
                                        req->ioVec().end() );

    loopSendReq( vec, m_nic->calcCoreId( calcNid( req, req->getDestRank() ) ), req );

    if ( ! req->isBlocking() ) {
        exit();
    } else {
        enterWait( new WaitReq( req ), m_exitDelay );
    }
}

void ProcessQueuesState::enterRecv( _CommReq* req, uint64_t exitDelay )
{
    dbg().verbose(CALL_INFO,1,1,"req=%p$ delay=%" PRIu64 "\n", req, exitDelay );
    m_exitDelay = exitDelay;

    dbg().verbose(CALL_INFO,1,1,"\n");
    if ( m_postedShortBuffers.size() < MaxPostedShortBuffers ) {
        if ( m_numNicRequestedShortBuff ) {
            --m_numNicRequestedShortBuff; 
        } else if ( m_numRecvLooped ) {
            --m_numRecvLooped;
        } else {
            postShortRecvBuffer();
        }
    }

    dbg().verbose(CALL_INFO,1,1,"\n");
    m_pstdRcvQ.push_front( req );

    m_statPstdRcv->addData( m_pstdRcvQ.size() );

    size_t length = req->getLength( );

    dbg().verbose(CALL_INFO,1,1,"\n");
    VoidFunction callback;

    if ( length > shortMsgLength() ) {
        callback = std::bind(
                &ProcessQueuesState::processRecv_0, this, req ); 
    } else {
        callback = std::bind(
                &ProcessQueuesState::processRecv_1, this, req );
    }

    dbg().verbose(CALL_INFO,1,1,"\n");
    schedCallback( callback, rxPostDelay_ns( length ) );
    dbg().verbose(CALL_INFO,1,1,"\n");
}

void ProcessQueuesState::processRecv_0( _CommReq* req )
{
    dbg().verbose(CALL_INFO,1,1,"\n");

    m_mem->pin(
        std::bind( &ProcessQueuesState::processRecv_1, this, req ),
        0, req->getLength() 
    );
}

void ProcessQueuesState::processRecv_1( _CommReq* req )
{
    if ( ! req->isBlocking() ) {
        exit();
    } else {
        enterWait( new WaitReq( req ), m_exitDelay );
    }
}

void ProcessQueuesState::enterMakeProgress(  uint64_t exitDelay  )
{
    dbg().verbose(CALL_INFO,1,1,"num pstd %lu, num short %lu\n",
                            m_pstdRcvQ.size(), m_recvdMsgQ.size() );

    m_exitDelay = exitDelay;

    WaitCtx* ctx = new WaitCtx ( NULL,
        std::bind( &ProcessQueuesState::processMakeProgress, this, &m_funcStack ) 
    );

    m_funcStack.push_back( ctx );

    processQueues( &m_funcStack );
}

void ProcessQueuesState::processMakeProgress( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu\n", stack->size()); 
    dbg().verbose(CALL_INFO,1,1,"num pstd %lu, num short %lu\n",
                            m_pstdRcvQ.size(), m_recvdMsgQ.size() );
    WaitCtx* ctx = static_cast<WaitCtx*>( stack->back() );

	delete ctx;
    stack->pop_back();

	assert( stack->empty() );

	exit();
}

void ProcessQueuesState::enterWait( WaitReq* req, uint64_t exitDelay  )
{
    dbg().verbose(CALL_INFO,1,1,"num pstd %lu, num short %lu\n",
                            m_pstdRcvQ.size(), m_recvdMsgQ.size() );

    m_exitDelay = exitDelay;

    WaitCtx* ctx = new WaitCtx ( req,
        std::bind( &ProcessQueuesState::processWait_0, this, &m_funcStack ) 
    );

    m_funcStack.push_back( ctx );

    processQueues( &m_funcStack );
}

void ProcessQueuesState::processWait_0( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu\n", stack->size()); 
    dbg().verbose(CALL_INFO,1,1,"num pstd %lu, num short %lu\n",
                            m_pstdRcvQ.size(), m_recvdMsgQ.size() );
    WaitCtx* ctx = static_cast<WaitCtx*>( stack->back() );

    stack->pop_back();

	assert( stack->empty() );

    _CommReq* req = ctx->req->getFiniReq();

    if ( req ) {
        processWaitCtx_0( ctx, req ); 
    } else {
        processWaitCtx_2( ctx );
    }
}

void ProcessQueuesState::processWaitCtx_0( WaitCtx* ctx, _CommReq* req )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    schedCallback( 
        std::bind( &ProcessQueuesState::processWaitCtx_1, this, ctx, req ),
        req->getFiniDelay()
    );
}

void ProcessQueuesState::processWaitCtx_1( WaitCtx* ctx, _CommReq* req )
{
    size_t   length = req->getLength(); 

    dbg().verbose(CALL_INFO,1,1,"\n");

    if ( length > shortMsgLength() ) {
        // this is a hack, to get point to point latencies to match Chama
        // we need to not add delay for unpinning the send buffer for long msg  
        if ( req->isSend() ) { 
            length = 0;
        } 
        m_mem->unpin( 
            std::bind( &ProcessQueuesState::processWaitCtx_2, this, ctx ),
            0, length 
        ); 
    } else {
        processWaitCtx_2( ctx );
    }

    if ( req->isMine() ) {
        delete req;
    }
}

void ProcessQueuesState::processWaitCtx_2( WaitCtx* ctx )
{
    _CommReq* req = ctx->req->getFiniReq();
    dbg().verbose(CALL_INFO,1,1,"\n");

    if ( req ) {
        processWaitCtx_0( ctx, req ); 
    } else  if ( ctx->req->isDone() ) {
        delete ctx;
        exit( );
    } else {
		enableInt( ctx, &ProcessQueuesState::processWait_0 );
    }
}

void ProcessQueuesState::processQueues( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"shortMsgV.size=%lu\n", m_recvdMsgQ.size() );
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu\n", stack->size()); 

    // this does not cost time
    while ( m_needRecv ) {
        ++m_numNicRequestedShortBuff; 
        postShortRecvBuffer();
        --m_needRecv;
    } 

    // this does not cost time
    while ( ! m_longAckQ.empty() ) {
        processLongAck( m_longAckQ.front() );
        m_longAckQ.pop_front();
    }

    // this does not cost time
    while ( ! m_loopResp.empty() ) {
        processLoopResp( m_loopResp.front() );
        m_loopResp.pop_front(); 
    }

    if ( ! m_longGetFiniQ.empty() ) {

        ProcessQueuesCtx* ctx = new ProcessQueuesCtx(
                bind( &ProcessQueuesState::processQueues0, this, stack )  
        ); 
        stack->push_back( ctx );

        processLongGetFini( stack, m_longGetFiniQ.front() );

        m_longGetFiniQ.pop_front();

    } else if ( ! m_recvdMsgQ.empty() ) {

        ProcessQueuesCtx* ctx = new ProcessQueuesCtx(
                bind ( &ProcessQueuesState::processQueues0, this, stack ) 
        );  
        stack->push_back( ctx );
        processShortList_0( stack );

    } else {
        schedCallback( stack->back()->getCallback() );
    }
}

void ProcessQueuesState::processQueues0( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu recvdMsgQ.size()=%lu\n", 
            stack->size(), m_recvdMsgQ.size() ); 
    
    delete stack->back();
    stack->pop_back();
    schedCallback( stack->back()->getCallback() );
}

void ProcessQueuesState::processShortList_0( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu recvdMsgQ.size()=%lu\n", 
            stack->size(), m_recvdMsgQ.size() ); 

    ProcessShortListCtx* ctx = new ProcessShortListCtx( m_recvdMsgQ );
	m_recvdMsgQ.clear();
    stack->push_back( ctx );

    processShortList_1( stack );
}

void ProcessQueuesState::processShortList_1( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu recvdMsgQ.size()=%lu\n", 
                        stack->size(), m_recvdMsgQ.size() ); 

    ProcessShortListCtx* ctx = 
                        static_cast<ProcessShortListCtx*>( stack->back() );
    
    int count = 0;
    ctx->req = searchPostedRecv( ctx->hdr(), count );

    m_mem->walk( 
        std::bind( &ProcessQueuesState::processShortList_2, this, stack ),
        count
    ); 
}

void ProcessQueuesState::processShortList_2( Stack* stack )
{
    ProcessShortListCtx* ctx = 
                        static_cast<ProcessShortListCtx*>( stack->back() );
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu\n", stack->size()); 

    if ( ctx->req ) {
        schedCallback( 
            std::bind( 
                    &ProcessQueuesState::processShortList_3, this, stack ),
            rxDelay( ctx->hdr().count * ctx->hdr().dtypeSize )
        ); 
    } else {
        ctx->incPos();
        processShortList_5( stack );
    }
}

void ProcessQueuesState::processShortList_3( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu\n", stack->size()); 

    ProcessShortListCtx* ctx = static_cast<ProcessShortListCtx*>(stack->back());

    _CommReq* req = ctx->req;

    req->setResp( ctx->hdr().tag, ctx->hdr().rank, ctx->hdr().count );

    size_t length = ctx->hdr().count * ctx->hdr().dtypeSize;

    if ( length <= shortMsgLength() || 
                            dynamic_cast<LoopReq*>( ctx->msg() ) ) 
    {
        dbg().verbose(CALL_INFO,2,1,"copyIoVec() short|loop message\n");

        copyIoVec( req->ioVec(), ctx->ioVec(), length );

        m_mem->copy( 
            std::bind(
                    &ProcessQueuesState::processShortList_4, this, stack ),
                1, 0, length 
        );

    } else {
        processShortList_4( stack );
    }
}

void ProcessQueuesState::processShortList_4( Stack* stack )
{
    ProcessShortListCtx* ctx = static_cast<ProcessShortListCtx*>(stack->back());

    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu\n", stack->size()); 

    _CommReq* req = ctx->req;

    size_t length = ctx->hdr().count * ctx->hdr().dtypeSize;

    LoopReq* loopReq;
    if ( ( loopReq = dynamic_cast<LoopReq*>( ctx->msg() ) ) ) {
        dbg().verbose(CALL_INFO,1,2,"loop message key=%p srcCore=%d "
            "srcRank=%d\n", loopReq->key, loopReq->srcCore, ctx->hdr().rank);
        req->setDone();
        loopSendResp( loopReq->srcCore , loopReq->key );

    } else if ( length <= shortMsgLength() ) { 
        dbg().verbose(CALL_INFO,1,1,"short\n");
        req->setDone( recvReqFiniDelay( length ) );
    } else {

        dbg().verbose(CALL_INFO,1,1,"long\n");
        VoidFunction* callback = new VoidFunction;
        *callback = std::bind( &ProcessQueuesState::getFini,this, req );

        nid_t nid = calcNid( ctx->req, ctx->hdr().rank );

        m_nic->get( nid, ctx->hdr().key, req->ioVec(), callback );

        req->m_ackKey = ctx->hdr().key; 
        req->m_ackNid = nid;
    }

    ctx->removeMsg();
    processShortList_5( stack );
}

void ProcessQueuesState::processShortList_5( Stack* stack )
{
    ProcessShortListCtx* ctx = static_cast<ProcessShortListCtx*>(stack->back());

    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu\n", stack->size()); 

    if ( ctx->isDone() ) {
        dbg().verbose(CALL_INFO,2,1,"return up the stack\n");

		if ( ! ctx->msgQempty() ) {
			m_recvdMsgQ.insert( m_recvdMsgQ.begin(), ctx->getMsgQ().begin(),
														ctx->getMsgQ().end() );
		}
        delete stack->back(); 
        stack->pop_back();
        schedCallback( stack->back()->getCallback() );
    } else {
        dbg().verbose(CALL_INFO,1,1,"work on next Msg\n");

        processShortList_1( stack );
    }
}

void ProcessQueuesState::processLoopResp( LoopResp* resp )
{
    dbg().verbose(CALL_INFO,1,2,"srcCore=%d\n",resp->srcCore );
    _CommReq* req = (_CommReq*)resp->key;  
    req->setDone();
    delete resp;
}

void ProcessQueuesState::getFini( _CommReq* req )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_longGetFiniQ.push_back( req );

    runInterruptCtx();
}

void ProcessQueuesState::dmaRecvFiniGI( GetInfo* info, uint64_t simVAddr, nid_t nid, 
                                            uint32_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,1,"nid=%d tag=%#x length=%lu\n",
                                                    nid, tag, length );
    assert( ( tag & LongAckKey ) == LongAckKey );
    m_simVAddrs->free( simVAddr );
    info->nid = nid;
    info->tag = tag;
    info->length = length;
    m_longAckQ.push_back( info );

    runInterruptCtx();
}

void ProcessQueuesState::dmaRecvFiniSRB( ShortRecvBuffer* buf, nid_t nid,
                                            uint32_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,1,"ShortMsgQ nid=%#x tag=%#x length=%lu\n",
                                                    nid, tag, length );
    dbg().verbose(CALL_INFO,1,1,"ShortMsgQ rank=%d tag=%#" PRIx64 " count=%d "
                            "dtypeSize=%d\n", buf->hdr.rank, buf->hdr.tag,
                            buf->hdr.count, buf->hdr.dtypeSize );

    assert( tag == (uint32_t) ShortMsgQ );
    m_recvdMsgQ.push_back( buf );
    m_statRcvdMsg->addData( m_recvdMsgQ.size() );

    runInterruptCtx();
    m_postedShortBuffers.erase(buf);
    dbg().verbose(CALL_INFO,1,1,"num postedShortRecvBuffers %lu\n",
                                        m_postedShortBuffers.size());
}

void ProcessQueuesState::enableInt( FuncCtxBase* ctx,
	void (ProcessQueuesState::*funcPtr)( Stack* ) )
{
  	dbg().verbose(CALL_INFO,2,1,"ctx=%p\n",ctx);
	assert( m_funcStack.empty() );
    if ( m_intCtx ) {
  	    dbg().verbose(CALL_INFO,2,1,"already have a return ctx\n");
        return; 
    }

	m_funcStack.push_back(ctx);

	ctx->setCallback( bind( funcPtr, this, &m_funcStack ) );

	m_intCtx = ctx;

	if ( m_missedInt ) runInterruptCtx();
}

void ProcessQueuesState::runInterruptCtx( )
{
	if ( ! m_intCtx || ! m_intStack.empty() ) {
    	dbg().verbose(CALL_INFO,2,1,"missed interrupt\n");
        m_missedInt = true;
		return;
	} 

    InterruptCtx* ctx = new InterruptCtx(
            std::bind( &ProcessQueuesState::leaveInterruptCtx, this, &m_intStack ) 
    ); 

    m_intStack.push_back( ctx );

	processQueues( &m_intStack );
}

void ProcessQueuesState::leaveInterruptCtx( Stack* stack )
{
    assert( 1 == stack->size() );
	delete stack->back();
	stack->pop_back();

    dbg().verbose(CALL_INFO,2,1,"\n" );

	VoidFunction callback = m_intCtx->getCallback();

	m_intCtx = NULL;

    callback();

    if ( m_intCtx && m_missedInt ) {
        runInterruptCtx();
        m_missedInt = false;
    }
}

void ProcessQueuesState::pioSendFiniVoid( void* hdr, uint64_t simVAddr )
{
    dbg().verbose(CALL_INFO,1,1,"hdr=%p\n", hdr );
    if ( hdr ) {
        free( hdr );
    }
    m_simVAddrs->free( simVAddr );
}

void ProcessQueuesState::pioSendFiniCtrlHdr( CtrlHdr* hdr, uint64_t simVAddr )
{
    dbg().verbose(CALL_INFO,1,1,"MsgHdr, Ack sent key=%#x\n", hdr->key);
	m_simVAddrs->free(simVAddr);
    delete hdr;
    runInterruptCtx();
}

void ProcessQueuesState::processLongGetFini( Stack* stack, _CommReq* req )
{
    ProcessLongGetFiniCtx* ctx = new ProcessLongGetFiniCtx( req );
    dbg().verbose(CALL_INFO,1,1,"\n");

    stack->push_back( ctx );

    schedCallback( 
        std::bind( &ProcessQueuesState::processLongGetFini0, this, stack),
        sendAckDelay() 
    );
}

void ProcessQueuesState::processLongGetFini0( Stack* stack )
{
    _CommReq* req = static_cast<ProcessLongGetFiniCtx*>(stack->back())->req;

    dbg().verbose(CALL_INFO,1,1,"\n");
    
    delete stack->back();
    stack->pop_back();

    req->setDone( recvReqFiniDelay( req->getLength() ) );

    IoVec hdrVec;   
    CtrlHdr* hdr = new CtrlHdr;
    hdrVec.len = sizeof( *hdr ); 
    hdrVec.addr.setSimVAddr( m_simVAddrs->alloc(hdrVec.len) );
    hdrVec.addr.setBacking( hdr ); 

    VoidFunction* callback = new VoidFunction;
    *callback = std::bind( 
                &ProcessQueuesState::pioSendFiniCtrlHdr, this, hdr, hdrVec.addr.getSimVAddr() ); 

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );

    dbg().verbose(CALL_INFO,1,1,"send long msg Ack to nid=%d key=%#x\n", 
                                                req->m_ackNid, req->m_ackKey );

    m_nic->pioSend( req->m_ackNid, req->m_ackKey, vec, callback );

    schedCallback( stack->back()->getCallback() );
}


void ProcessQueuesState::processLongAck( GetInfo* info )
{
    dbg().verbose(CALL_INFO,1,1,"acked\n");
    info->req->setDone( sendReqFiniDelay( info->req->getLength() ) );
    delete info;
    return;
}

void ProcessQueuesState::needRecv( int nid, size_t length )
{
    dbg().verbose(CALL_INFO,1,1,"nid=%d length=%lu\n",nid,length);

    ++m_needRecv;
    runInterruptCtx();
}

void ProcessQueuesState::loopHandler( int srcCore, void* key )
{
    dbg().verbose(CALL_INFO,1,2,"resp: srcCore=%d key=%p \n",srcCore,key);

    m_loopResp.push_back( new LoopResp( srcCore, key ) );

    runInterruptCtx();
}

void ProcessQueuesState::loopHandler( int srcCore, std::vector<IoVec>& vec, void* key )
{
        
    MatchHdr* hdr = (MatchHdr*) vec[0].addr.getBacking();

    dbg().verbose(CALL_INFO,1,2,"req: srcCore=%d key=%p vec.size()=%lu srcRank=%d\n",
                                                    srcCore, key, vec.size(), hdr->rank);

    ++m_numRecvLooped;
    m_recvdMsgQ.push_back( new LoopReq( srcCore, vec, key ) );
    m_statRcvdMsg->addData( m_recvdMsgQ.size() );

    runInterruptCtx();
}

_CommReq* ProcessQueuesState::searchPostedRecv( MatchHdr& hdr, int& count )
{
    _CommReq* req = NULL;
    dbg().verbose(CALL_INFO,1,1,"posted size %lu\n",m_pstdRcvQ.size());

    std::deque< _CommReq* >:: iterator iter = m_pstdRcvQ.begin();
    for ( ; iter != m_pstdRcvQ.end(); ++iter ) {

        ++count;
        if ( ! checkMatchHdr( hdr, (*iter)->hdr(), (*iter)->ignore() ) ) {
            continue;
        }

        req = *iter;
        m_pstdRcvQ.erase(iter);
        break;
    }
    dbg().verbose(CALL_INFO,2,1,"req=%p\n",req);

    return req;
}

bool ProcessQueuesState::checkMatchHdr( MatchHdr& hdr, MatchHdr& wantHdr,
                                    uint64_t ignore )
{
    dbg().verbose(CALL_INFO,1,1,"posted tag %#" PRIx64 ", msg tag %#" PRIx64 "\n", wantHdr.tag, hdr.tag );
    if ( ( AnyTag != wantHdr.tag ) && 
            ( ( wantHdr.tag & ~ignore) != ( hdr.tag & ~ignore ) ) ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,1,"want rank %d %d\n", wantHdr.rank, hdr.rank );
    if ( ( MP::AnySrc != wantHdr.rank ) && ( wantHdr.rank != hdr.rank ) ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,1,"want group %d %d\n", wantHdr.group,hdr.group);
    if ( wantHdr.group != hdr.group ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,1,"want count %d %d\n", wantHdr.count,
                                    hdr.count);
    if ( wantHdr.count !=  hdr.count ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,1,"want dtypeSize %d %d\n", wantHdr.dtypeSize,
                                    hdr.dtypeSize);
    if ( wantHdr.dtypeSize !=  hdr.dtypeSize ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,1,"matched\n");
    return true;
}

void ProcessQueuesState::copyIoVec( 
                std::vector<IoVec>& dst, std::vector<IoVec>& src, size_t len )
{
    dbg().verbose(CALL_INFO,1,1,"dst.size()=%lu src.size()=%lu wantLen=%lu\n",
                                    dst.size(), src.size(), len );

    size_t copied = 0;
    size_t rV = 0,rP =0;
    for ( unsigned int i=0; i < src.size() && copied < len; i++ ) 
    {
        assert( rV < dst.size() );
        dbg().verbose(CALL_INFO,3,1,"src[%d].len %lu\n", i, src[i].len);

        for ( unsigned int j=0; j < src[i].len && copied < len ; j++ ) {
            dbg().verbose(CALL_INFO,3,1,"copied=%lu rV=%lu rP=%lu\n",
                                                        copied,rV,rP);

            if ( dst[rV].addr.getBacking() && src[i].addr.getBacking() ) {
                ((char*)dst[rV].addr.getBacking())[rP] = 
                            ((char*)src[i].addr.getBacking())[j];
            }
            ++copied;
            ++rP;
            if ( rP == dst[rV].len ) {
                rP = 0;
                ++rV;
            } 
        }
    }
    assert( copied == len );
}

void ProcessQueuesState::postShortRecvBuffer( )
{
    ShortRecvBuffer* buf =
            new ShortRecvBuffer( shortMsgLength() + sizeof(MatchHdr), *m_simVAddrs );

    Callback2* callback = new Callback2;
    *callback = std::bind( 
        &ProcessQueuesState::dmaRecvFiniSRB, this, buf, 
        std::placeholders::_1,
        std::placeholders::_2, 
        std::placeholders::_3
    ); 

    // save this info so we can cleanup in the destructor 
    m_postedShortBuffers[buf ] = callback;

    dbg().verbose(CALL_INFO,1,1,"num postedShortRecvBuffers %lu\n",
                                        m_postedShortBuffers.size());
    m_nic->dmaRecv( -1, ShortMsgQ, buf->ioVec, callback ); 
}

void ProcessQueuesState::loopSendReq( std::vector<IoVec>& vec, int core, void* key )
{
    m_dbg.verbose(CALL_INFO,1,1,"dest core=%d key=%p\n",core,key);

    m_loopLink->send(0, new LoopBackEvent( vec, core, key ) );
}

void ProcessQueuesState::loopSendResp( int core, void* key )
{
    m_dbg.verbose(CALL_INFO,1,1,"dest core=%d key=%p\n",core,key);
    m_loopLink->send(0, new LoopBackEvent( core, key ) );
}

void ProcessQueuesState::loopHandler( Event* ev )
{
    LoopBackEvent* event = static_cast< LoopBackEvent* >(ev);
    m_dbg.verbose(CALL_INFO,1,1,"%s key=%p\n",
        event->vec.empty() ? "Response" : "Request", event->key);

    if ( event->vec.empty() ) {
        loopHandler(event->core, event->key );
    } else {
        loopHandler(event->core, event->vec, event->key);
    }
    delete ev;
}

void ProcessQueuesState::delayHandler( SST::Event* e )
{
    DelayEvent* event = static_cast<DelayEvent*>(e);

    m_dbg.verbose(CALL_INFO,2,1,"execute callback\n");

    event->callback();
    delete e;
}
