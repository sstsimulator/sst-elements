// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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


ProcessQueuesState::ProcessQueuesState( ComponentId_t id, Params& params ) :
        SubComponent( id ),

        m_getKey( 0 ),
        m_rspKey( 0 ),
        m_needRecv( 0 ),
        m_numNicRequestedShortBuff(0),
        m_numRecvLooped(0),
        m_missedInt( false ),
        m_intCtx(NULL),
		m_simVAddrs(NULL),
        m_numSent(0),
        m_numRecv(0),
        m_rendezvousVN(0),
        m_ackVN(0),
        m_recvdMsgQ(2),
        m_recvdMsgQpos(0)
{
    int level = params.find<uint32_t>("pqs.verboseLevel",0);
    int mask = params.find<int32_t>("pqs.verboseMask",-1);
    m_nicsPerNode = params.find<int32_t>("nicsPerNode",1);
    m_maxUnexpectedMsg = params.find<int32_t>("pqs.maxUnexpectedMsg",32);
    m_maxPostedShortBuffers = params.find<int32_t>("pqs.maxPostedShortBuffers",512); 
    m_minPostedShortBuffers = params.find<int32_t>("pqs.minPostedShortBuffers",5); 

    m_dbg.init("", level, mask, Output::STDOUT );

    m_statPstdRcv = registerStatistic<uint64_t>("posted_receive_list");
    m_statRcvdMsg = registerStatistic<uint64_t>("received_msg_list");

    m_msgTiming = loadAnonymousSubComponent< MsgTiming >( "firefly.msgTiming", "", 0, ComponentInfo::SHARE_NONE, params );

    std::stringstream ss;
    ss << this;

    m_delayLink = configureSelfLink(
                        "ProcessQueuesStateSelfLink." + ss.str(), "1 ns",
                                new Event::Handler<ProcessQueuesState>(this,&ProcessQueuesState::delayHandler));

    m_loopLink = configureLink(
            params.find<std::string>("loopBackPortName", "loop"), "1 ns",
            new Event::Handler<ProcessQueuesState>(this,&ProcessQueuesState::loopHandler) );
    assert(m_loopLink);

    m_ackVN = params.find<int>( "ackVN", 0 );
    m_rendezvousVN = params.find<int>( "rendezvousVN", 0 );
}


ProcessQueuesState::~ProcessQueuesState()
{
    while ( ! m_postedShortBuffers.empty() ) {
        delete m_postedShortBuffers.begin()->first;
        delete m_postedShortBuffers.begin()->second;
        m_postedShortBuffers.erase( m_postedShortBuffers.begin() );
    }
	if ( m_simVAddrs ) {
    	delete m_simVAddrs;
	}
    delete m_msgTiming;
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
    snprintf(buffer,100,"@t:%d:%d:CtrlMsg::ProcessQueuesState::@p():@l ",
                            m_nic->getRealNodeId(), m_info->worldRank());
    dbg().setPrefix(buffer);
    dbg().debug(CALL_INFO,1,1,"%s\n",m_memHeapLink ? "use memHeap":"\n");
}

void ProcessQueuesState:: finish() {
    dbg().debug(CALL_INFO,1,1,"pstdRcvQ=%lu recvdMsgQ=%s loopResp=%lu funcStack=%lu sent=%d recv=%d\n",
    m_pstdRcvQ.size(), recvdMsgQsize(), m_loopResp.size(), m_funcStack.size(), m_numSent, m_numRecv );
}

void ProcessQueuesState::enterInit( bool haveGlobalMemHeap )
{
    dbg().debug(CALL_INFO,1,1,"%s\n",haveGlobalMemHeap?"use global memory heap":"");

    size_t length = 0;
    length += ( m_maxPostedShortBuffers + m_maxUnexpectedMsg ) * (sizeof(MatchHdr) + 16 ) & ~15;
    length += ( m_maxPostedShortBuffers + m_maxUnexpectedMsg ) * (shortMsgLength() + 16 ) & ~15;

    std::function<void(uint64_t)> callback = [=](uint64_t addr){
		enterInit_1( addr, length );
        return 0;
    };

    if ( haveGlobalMemHeap ) {
        memHeapAlloc( length, callback );
    } else {
        enterInit_1( 0xf0000000, length );
    }
}

void ProcessQueuesState::enterInit_1( uint64_t addr, size_t length )
{
    dbg().debug(CALL_INFO,1,1,"simVAddr %" PRIx64 "  length=%zu\n", addr, length );

    m_simVAddrs = new HeapAddrs( addr, length );

    for ( unsigned long i = 0; i < m_minPostedShortBuffers; i++ ) {
        postShortRecvBuffer();
    }

    passCtrlToFunction( );
}

void ProcessQueuesState::enterSend( _CommReq* req, uint64_t exitDelay )
{
    m_exitDelay = exitDelay;
    req->setSrcRank( getMyRank( req ) );
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_APP_SIDE,"req=%p delay=%" PRIu64 " destRank=%d\n", req, exitDelay,
				req->getDestRank() );

    uint64_t delay = txDelay( req->getLength() );

    VoidFunction callback;
    if ( m_nic->isLocal( calcNid( req, req->getDestRank()), m_nicsPerNode ) ) {
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
    size_t length = req->getLength( );
    int vn = 0;

	std::vector<void*> ptrs;
    IoVec hdrVec;
    hdrVec.len = sizeof( req->hdr() );

    hdrVec.addr.setSimVAddr( m_simVAddrs->alloc( hdrVec.len ) );

    if ( length <= shortMsgLength() ) {
		hdrVec.addr.setBacking( malloc( hdrVec.len ) );
		ptrs.push_back ( hdrVec.addr.getBacking() );
        memcpy( hdrVec.addr.getBacking(), &req->hdr(), hdrVec.len );
    } else {
        hdrVec.addr.setBacking( &req->hdr() );
    }

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );

    nid_t nid = calcNid( req, req->getDestRank() );

    if ( length <= shortMsgLength() ) {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"Short %lu bytes dest %#x\n",length,nid);
		for ( int i = 0; i < req->ioVec().size(); i++ ) {
			size_t len = req->ioVec()[i].len;
			MemAddr& addr =	req->ioVec()[i].addr;
			void* backing = NULL;
			if ( addr.getBacking() ) {
				backing = malloc( len );
				memcpy( backing, addr.getBacking(), len );
			}
			vec.push_back( IoVec( MemAddr( addr.getSimVAddr(), backing),len ) );
		}
        req->setDone( sendReqFiniDelay( length ) );
        ++m_numSent;
        vn = req->m_vn;

    } else {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"sending long message %lu bytes\n",length);
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
        vn = m_rendezvousVN;
    }

    VoidFunction* callback = new VoidFunction;
    *callback = std::bind( &ProcessQueuesState::pioSendFiniVoid, this, ptrs, hdrVec.addr.getSimVAddr() );

    m_nic->pioSend( vn, nid, ShortMsgQ, vec, callback);

    if ( ! req->isBlocking() ) {
        enterMakeProgress(m_exitDelay);
    } else {
        enterWait( new WaitReq( req ), m_exitDelay );
    }
}

void ProcessQueuesState::processSendLoop( _CommReq* req )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"key=%p\n", req);

    IoVec hdrVec;
    hdrVec.len = sizeof( req->hdr() );
    hdrVec.addr.setSimVAddr( 1 ); //m_simVAddrs->alloc( hdrVec.len );
    hdrVec.addr.setBacking( &req->hdr() );

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );
    vec.insert( vec.begin() + 1, req->ioVec().begin(),
                                        req->ioVec().end() );

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"destCore=%d\n", m_nic->calcCoreId( calcNid( req, req->getDestRank() ), m_nicsPerNode) );
    loopSendReq( vec, m_nic->calcCoreId( calcNid( req, req->getDestRank() ), m_nicsPerNode ), req );

    if ( ! req->isBlocking() ) {
        enterMakeProgress(m_exitDelay);
    } else {
        enterWait( new WaitReq( req ), m_exitDelay );
    }
}

void ProcessQueuesState::enterRecv( _CommReq* req, uint64_t exitDelay )
{
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_APP_SIDE,"req=%p delay=%" PRIu64 " rank=%d\n", req, exitDelay, req->hdr().rank );
    m_exitDelay = exitDelay;

    if ( m_postedShortBuffers.size() < m_maxPostedShortBuffers ) {
        if ( m_numNicRequestedShortBuff ) {
            --m_numNicRequestedShortBuff;
        } else if ( m_numRecvLooped ) {
            --m_numRecvLooped;
        } else {
            postShortRecvBuffer();
        }
    }

    m_statPstdRcv->addData( m_pstdRcvQ.size() );

    size_t length = req->getLength( );

    VoidFunction callback;

    if ( length > shortMsgLength() ) {
        callback = std::bind(
                &ProcessQueuesState::processRecv_0, this, req );
    } else {
        callback = std::bind(
                &ProcessQueuesState::processRecv_1, this, req );
    }

    schedCallback( callback, rxPostDelay_ns( length ) );
}

void ProcessQueuesState::processRecv_0( _CommReq* req )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"\n");

    m_mem->pin(
        std::bind( &ProcessQueuesState::processRecv_1, this, req ),
        0, req->getLength()
    );
}

void ProcessQueuesState::processRecv_1( _CommReq* req )
{
    if ( ! m_unexpectedMsgQ.empty() ) {

        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"check unexpected queue\n");

        assert( m_pstdRcvPreQ.empty() );
        m_pstdRcvPreQ.push_back( req );

        ProcessQueuesCtx* ctx = new ProcessQueuesCtx(
            std::bind( &ProcessQueuesState::processRecv_2, this, &m_funcStack, req )
        );

        m_funcStack.push_back( ctx );
        processShortList_0( &m_funcStack );
    } else {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"post receive\n");
        m_pstdRcvQ.push_back( req );
        processRecv_2( NULL, req );
    }
}

void ProcessQueuesState::processRecv_2( Stack* stack, _CommReq* req )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"\n");
    if ( stack ) {
        delete stack->back();
        stack->pop_back();
    }

    if ( ! m_pstdRcvPreQ.empty() ) {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"no match against unexpected queue move to pstRecvQ\n");
        m_pstdRcvQ.push_back( m_pstdRcvPreQ.front() );
        m_pstdRcvPreQ.clear();
    }

    if ( ! req->isBlocking() || req->isDone() ) {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"exit\n");
		enterMakeProgress( m_exitDelay );
    } else {
        WaitCtx* ctx = new WaitCtx ( new WaitReq(req),
            std::bind( &ProcessQueuesState::processWait_0, this, &m_funcStack )
        );
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"enable interrupts\n");
        enableInt( ctx, &ProcessQueuesState::processWait_0 );
    }
}

void ProcessQueuesState::enterMakeProgress(  uint64_t exitDelay  )
{
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_APP_SIDE,"num pstd %lu, rcvdMsgQ %s\n",
                            m_pstdRcvQ.size(), recvdMsgQsize() );

    m_exitDelay = exitDelay;

    if ( m_recvdMsgQ[m_recvdMsgQpos].empty() ) {
        exit();
    } else {
        WaitCtx* ctx = new WaitCtx ( NULL,
            std::bind( &ProcessQueuesState::processMakeProgress, this, &m_funcStack )
        );

        enableInt( ctx, &ProcessQueuesState::processMakeProgress );
    }
}

void ProcessQueuesState::processMakeProgress( Stack* stack )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"stack.size()=%lu\n", stack->size());
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"num pstd %lu, recvdMsgQ %s\n", m_pstdRcvQ.size(), recvdMsgQsize() );
    WaitCtx* ctx = static_cast<WaitCtx*>( stack->back() );

	delete ctx;
    stack->pop_back();

	assert( stack->empty() );

	exit();
}

void ProcessQueuesState::enterCancel( MP::MessageRequest req, uint64_t exitDelay ) {

    std::deque< _CommReq* >:: iterator iter = m_pstdRcvQ.begin();
    for ( ; iter != m_pstdRcvQ.end(); ++iter ) {

		if ( *iter == req ) {
    		dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"found req=%p\n",*iter);
			delete( *iter );
        	m_pstdRcvQ.erase(iter);
			break;
		}
    }
    enterMakeProgress(m_exitDelay);
}

void ProcessQueuesState::enterTest( WaitReq* req, int* flag, uint64_t exitDelay  )
{
	dbg().debug(CALL_INFO,1,DBG_MSK_PQS_APP_SIDE,"\n");

    m_exitDelay = exitDelay;

    WaitCtx* ctx = new WaitCtx ( req,
        std::bind( &ProcessQueuesState::processWait_0, this, &m_funcStack ),
		flag
    );

    m_funcStack.push_back( ctx );

    processWait_0( &m_funcStack );
}

void ProcessQueuesState::enterWait( WaitReq* req, uint64_t exitDelay  )
{
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_APP_SIDE,"num pstd %lu, recvdMsgQ %s\n", m_pstdRcvQ.size(), recvdMsgQsize() );

    m_exitDelay = exitDelay;

    WaitCtx* ctx = new WaitCtx ( req,
        std::bind( &ProcessQueuesState::processWait_0, this, &m_funcStack )
    );

    m_funcStack.push_back( ctx );

    processWait_0( &m_funcStack );
}

void ProcessQueuesState::processWait_0( Stack* stack )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"stack.size()=%lu num pstd %lu, recvdMsgQ %s\n",
        stack->size(), m_pstdRcvQ.size(), recvdMsgQsize() );

    WaitCtx* ctx = static_cast<WaitCtx*>( stack->back() );

    stack->pop_back();

	assert( stack->empty() );

    _CommReq* req = ctx->req->getFiniReq();

    // a part of this WaitCtx is done
    if ( req ) {
        processWaitCtx_0( ctx, req );
    // the wait request is complete
    } else if ( ctx->flag ) {
        delete ctx;
        enterMakeProgress(m_exitDelay);
    } else {
        // WaitCtx is blocked
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"blocking call enableInt() return to processWait_0()\n");
        enableInt( ctx, &ProcessQueuesState::processWait_0 );
    }
}

void ProcessQueuesState::processWaitCtx_0( WaitCtx* ctx, _CommReq* req )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"\n");
    schedCallback(
        std::bind( &ProcessQueuesState::processWaitCtx_1, this, ctx, req ),
        req->getFiniDelay()
    );
}

void ProcessQueuesState::processWaitCtx_1( WaitCtx* ctx, _CommReq* req )
{
    size_t   length = req->getLength();

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"\n");

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
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"\n");

    if ( req ) {
        processWaitCtx_0( ctx, req );
    } else  if ( ctx->req->isDone() ) {
        if ( ctx->flag ) {
            dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"set flag\n");
            *ctx->flag = 1;
        }
        delete ctx;
        enterMakeProgress(m_exitDelay);
    } else {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"blocking call enableInt() return to processWait_0()\n");
		enableInt( ctx, &ProcessQueuesState::processWait_0 );
    }
}

void ProcessQueuesState::processQueues( Stack* stack )
{
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_Q,"recvdMsgQ=%s m_unexpectedMsgQ=%zu m_pstdRecvPre=%zu m_pstdRcvQ=%zu\n",
							recvdMsgQsize(), m_unexpectedMsgQ.size(), m_pstdRcvPreQ.size(), m_pstdRcvQ.size() );
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_Q,"stack.size()=%lu\n", stack->size());

    assert ( ! m_intStack.empty() );

    while ( m_needRecv && m_numNicRequestedShortBuff < m_maxUnexpectedMsg ) {
        dbg().debug(CALL_INFO,1,DBG_MSK_PQS_Q,"post buff for needRecv %d m_numNicRequestedShortBuff=%d\n",
									m_needRecv, m_numNicRequestedShortBuff);

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

    } else if ( ! m_recvdMsgQ[m_recvdMsgQpos].empty()  ) {

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
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"stack.size()=%lu recvdMsgQ=%s\n",
            stack->size(), recvdMsgQsize() );

    delete stack->back();
    stack->pop_back();
    schedCallback( stack->back()->getCallback() );
}

void ProcessQueuesState::processShortList_0( Stack* stack )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"stack.size()=%lu recvdMsgQ=%s\n",
            stack->size(), recvdMsgQsize() );

    ProcessShortListCtx* ctx;
    if ( m_intStack.empty() ) {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"use unexpectedMsgQ %zu\n",m_unexpectedMsgQ.size());
        ctx = new ProcessShortListCtx( &m_unexpectedMsgQ );
    } else {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"use recvdMsgQ pos=%d\n",m_recvdMsgQpos);
        ctx = new ProcessShortListCtx( &m_recvdMsgQ[m_recvdMsgQpos] );
        ++m_recvdMsgQpos;
        m_recvdMsgQpos %= 2;
    }

    stack->push_back( ctx );

    processShortList_1( stack );
}

void ProcessQueuesState::processShortList_1( Stack* stack )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"stack.size()=%lu recvdMsgQ=%s\n",
                        stack->size(), recvdMsgQsize() );

    ProcessShortListCtx* ctx =
                        static_cast<ProcessShortListCtx*>( stack->back() );

    int count = 0;
    if ( m_intStack.empty() ) {
        ctx->req = searchPostedRecv( m_pstdRcvPreQ, ctx->hdr(), count );
    } else {
        ctx->req = searchPostedRecv( m_pstdRcvQ, ctx->hdr(), count );
    }

    m_mem->walk(
        std::bind( &ProcessQueuesState::processShortList_2, this, stack ),
        count
    );
}

void ProcessQueuesState::processShortList_2( Stack* stack )
{
    ProcessShortListCtx* ctx =
                        static_cast<ProcessShortListCtx*>( stack->back() );
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"stack.size()=%lu\n", stack->size());

    if ( ctx->req ) {
        schedCallback(
            std::bind(
                    &ProcessQueuesState::processShortList_3, this, stack ),
            rxDelay( ctx->hdr().count * ctx->hdr().dtypeSize )
        );
    } else {
        if ( m_intStack.empty() ) {
            ctx->incPos();
        } else {
            m_unexpectedMsgQ.push_back( ctx->msg() );
            ctx->unlinkMsg();
        }
        processShortList_5( stack );
    }
}

void ProcessQueuesState::processShortList_3( Stack* stack )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"stack.size()=%lu\n", stack->size());

    ProcessShortListCtx* ctx = static_cast<ProcessShortListCtx*>(stack->back());

    _CommReq* req = ctx->req;

    req->setResp( ctx->hdr().tag, ctx->hdr().rank, ctx->hdr().count, ctx->hdr().dtypeSize );

    size_t length = ctx->hdr().count * ctx->hdr().dtypeSize;

    if ( length <= shortMsgLength() ||
                            dynamic_cast<LoopReq*>( ctx->msg() ) )
    {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"copyIoVec() short|loop message\n");

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

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"stack.size()=%lu\n", stack->size());

    _CommReq* req = ctx->req;

    size_t length = ctx->hdr().count * ctx->hdr().dtypeSize;

    LoopReq* loopReq;
    if ( ( loopReq = dynamic_cast<LoopReq*>( ctx->msg() ) ) ) {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"loop message key=%p srcCore=%d "
            "srcRank=%d\n", loopReq->key, loopReq->srcCore, ctx->hdr().rank);
        req->setDone();
        ++m_numRecv;
        loopSendResp( loopReq->srcCore , loopReq->key );

    } else if ( length <= shortMsgLength() ) {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"short req=%p\n", req);
        req->setDone( recvReqFiniDelay( length ) );
        ++m_numRecv;
    } else {

        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"long\n");
        VoidFunction* callback = new VoidFunction;
        *callback = std::bind( &ProcessQueuesState::getFini,this, req );

        nid_t nid = calcNid( ctx->req, ctx->hdr().rank );

        m_nic->get( nid, ctx->hdr().key, req->ioVec(), callback );

        req->m_ackKey = ctx->hdr().key;
        req->m_ackNid = nid;
    }

    ctx->removeMsg();
    if ( m_intStack.empty() ) {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"set done\n");
        ctx->setDone();
    }
    processShortList_5( stack );
}

void ProcessQueuesState::processShortList_5( Stack* stack )
{
    ProcessShortListCtx* ctx = static_cast<ProcessShortListCtx*>(stack->back());

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"stack.size()=%lu\n", stack->size());

    if ( ctx->isDone() ) {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"return up the stack\n");

        delete stack->back();
        stack->pop_back();
        schedCallback( stack->back()->getCallback() );
    } else {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"work on next Msg\n");

        processShortList_1( stack );
    }
}

void ProcessQueuesState::processLoopResp( LoopResp* resp )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"srcCore=%d\n",resp->srcCore );
    _CommReq* req = (_CommReq*)resp->key;
    ++m_numSent;
    req->setDone();
    delete resp;
}

void ProcessQueuesState::getFini( _CommReq* req )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_CB,"\n");
    m_longGetFiniQ.push_back( req );

    runInterruptCtx();
}

void ProcessQueuesState::dmaRecvFiniGI( GetInfo* info, uint64_t simVAddr, nid_t nid,
                                            uint32_t tag, size_t length )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_CB,"nid=%d tag=%#x length=%lu\n",
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
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_CB,"ShortMsgQ nid=%#x tag=%#x length=%lu\n",
                                                    nid, tag, length );
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_CB,"ShortMsgQ rank=%d tag=%#" PRIx64 " count=%d "
                            "dtypeSize=%d\n", buf->hdr.rank, buf->hdr.tag,
                            buf->hdr.count, buf->hdr.dtypeSize );

    assert( tag == (uint32_t) ShortMsgQ );
    m_recvdMsgQ[m_recvdMsgQpos].push_back( buf );
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_CB,"recvdMsgQ=%s\n",recvdMsgQsize());
    m_statRcvdMsg->addData( m_recvdMsgQ.size() );

    runInterruptCtx();
    m_postedShortBuffers.erase(buf);
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_CB,"num postedShortRecvBuffers %lu\n",
                                        m_postedShortBuffers.size());
}

void ProcessQueuesState::enableInt( FuncCtxBase* ctx,
	void (ProcessQueuesState::*funcPtr)( Stack* ) )
{
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT,"ctx=%p missedInt=%d\n",ctx,m_missedInt);
    assert( m_funcStack.empty() );
    if ( m_intCtx ) {
  	    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT,"already have a return ctx\n");
        return;
    }

    m_funcStack.push_back(ctx);

    ctx->setCallback( bind( funcPtr, this, &m_funcStack ) );

    m_intCtx = ctx;

    if ( m_missedInt )  {
        runInterruptCtx();
    } else {
        dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT,"nothing to do\n");
    }
}

void ProcessQueuesState::runInterruptCtx( )
{
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT," %p %zu\n",m_intCtx, m_intStack.size());

	if ( ! m_intCtx || ! m_intStack.empty() ) {
    	dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT,"missed interrupt\n");
        m_missedInt = true;
		return;
	}

	// we are now in interrupt context 

    InterruptCtx* ctx = new InterruptCtx(
            std::bind( &ProcessQueuesState::leaveInterruptCtx, this, &m_intStack )
    );

    m_intStack.push_back( ctx );
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT,"m_intStack.size()=%zu\n",m_intStack.size());

	// clear the missed interrupt flag 
	m_missedInt = false;
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT,"call processQueues\n" );
	processQueues( &m_intStack );
}

void ProcessQueuesState::leaveInterruptCtx( Stack* stack )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_CB,"recvdMsgQ=%s\n",recvdMsgQsize());
    assert( 1 == stack->size() );
	delete stack->back();
	stack->pop_back();

    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT,"\n" );

	VoidFunction callback = m_intCtx->getCallback();

	m_intCtx = NULL;

    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT,"before \n" );
	// this should send us back to user context
    callback();
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_INT,"after \n" );

#if 0 // do we need this?
    if ( m_intCtx && m_missedInt ) {
        runInterruptCtx();
        m_missedInt = false;
    }
#endif
}

void ProcessQueuesState::pioSendFiniVoid( std::vector<void*> ptrs, uint64_t simVAddr )
{
	for ( int i = 0; i < ptrs.size(); i++ ) {
    	dbg().debug(CALL_INFO,1,DBG_MSK_PQS_CB,"ptr=%p\n", ptrs[0] );
		free( ptrs[i] );
	}
    m_simVAddrs->free( simVAddr );
}

void ProcessQueuesState::pioSendFiniCtrlHdr( CtrlHdr* hdr, uint64_t simVAddr )
{
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_CB,"MsgHdr, Ack sent key=%#x\n", hdr->key);
	m_simVAddrs->free(simVAddr);
    delete hdr;
    runInterruptCtx();
}

void ProcessQueuesState::processLongGetFini( Stack* stack, _CommReq* req )
{
    ProcessLongGetFiniCtx* ctx = new ProcessLongGetFiniCtx( req );
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_CB,"\n");

    stack->push_back( ctx );

    schedCallback(
        std::bind( &ProcessQueuesState::processLongGetFini0, this, stack),
        sendAckDelay()
    );
}

void ProcessQueuesState::processLongGetFini0( Stack* stack )
{
    _CommReq* req = static_cast<ProcessLongGetFiniCtx*>(stack->back())->req;

    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_CB,"\n");

    delete stack->back();
    stack->pop_back();

    ++m_numRecv;
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

    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_CB,"send long msg Ack to nid=%d key=%#x\n",
                                                req->m_ackNid, req->m_ackKey );

    m_nic->pioSend( m_ackVN, req->m_ackNid, req->m_ackKey, vec, callback );

    schedCallback( stack->back()->getCallback() );
}


void ProcessQueuesState::processLongAck( GetInfo* info )
{
    ++m_numSent;
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"acked\n");
    info->req->setDone( sendReqFiniDelay( info->req->getLength() ) );
    delete info;
    return;
}

void ProcessQueuesState::needRecv( int nid, size_t length )
{
    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_NEED_RECV,"nid=%d length=%lu\n",nid,length);

    ++m_needRecv;
    runInterruptCtx();
}

void ProcessQueuesState::loopHandler( int srcCore, void* key )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_CB,"resp: srcCore=%d key=%p \n",srcCore,key);

    m_loopResp.push_back( new LoopResp( srcCore, key ) );

    runInterruptCtx();
}

void ProcessQueuesState::loopHandler( int srcCore, std::vector<IoVec>& vec, void* key )
{

    MatchHdr* hdr = (MatchHdr*) vec[0].addr.getBacking();

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_CB,"req: srcCore=%d key=%p vec.size()=%lu srcRank=%d\n",
                                                    srcCore, key, vec.size(), hdr->rank);

    ++m_numRecvLooped;
    m_recvdMsgQ[m_recvdMsgQpos].push_back( new LoopReq( srcCore, vec, key ) );
    m_statRcvdMsg->addData( m_recvdMsgQ.size() );

    runInterruptCtx();
}

_CommReq* ProcessQueuesState::searchPostedRecv( std::deque< _CommReq* >& pstd, MatchHdr& hdr, int& count )
{
    _CommReq* req = NULL;
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"posted size %lu\n",pstd.size());

    std::deque< _CommReq* >:: iterator iter = pstd.begin();
    for ( ; iter != pstd.end(); ++iter ) {

        ++count;
        if ( ! checkMatchHdr( hdr, (*iter)->hdr(), (*iter)->ignore() ) ) {
            continue;
        }

        req = *iter;
        pstd.erase(iter);
        break;
    }
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"req=%p\n",req);

    return req;
}

bool ProcessQueuesState::checkMatchHdr( MatchHdr& hdr, MatchHdr& wantHdr,
                                    uint64_t ignore )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"posted tag %#" PRIx64 ", msg tag %#" PRIx64 "\n", wantHdr.tag, hdr.tag );
    if ( ( AnyTag != wantHdr.tag ) &&
            ( ( wantHdr.tag & ~ignore) != ( hdr.tag & ~ignore ) ) ) {
        return false;
    }

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"want rank %d %d\n", wantHdr.rank, hdr.rank );
    if ( ( MP::AnySrc != wantHdr.rank ) && ( wantHdr.rank != hdr.rank ) ) {
        return false;
    }

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"want group %d %d\n", wantHdr.group,hdr.group);
    if ( wantHdr.group != hdr.group ) {
        return false;
    }

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"want count %d %d\n", wantHdr.count,
                                    hdr.count);
    if ( wantHdr.count !=  hdr.count ) {
        return false;
    }

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"want dtypeSize %d %d\n", wantHdr.dtypeSize,
                                    hdr.dtypeSize);
    if ( wantHdr.dtypeSize !=  hdr.dtypeSize ) {
        return false;
    }

    dbg().debug(CALL_INFO,1,DBG_MSK_PQS_Q,"matched\n");
    return true;
}

void ProcessQueuesState::copyIoVec(
                std::vector<IoVec>& dst, std::vector<IoVec>& src, size_t len )
{
    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_Q,"dst.size()=%lu src.size()=%lu wantLen=%lu\n",
                                    dst.size(), src.size(), len );

    size_t copied = 0;
    size_t rV = 0,rP =0;
    for ( unsigned int i=0; i < src.size() && copied < len; i++ )
    {
        assert( rV < dst.size() );
        dbg().debug(CALL_INFO,3,DBG_MSK_PQS_Q,"src[%d].len %lu\n", i, src[i].len);

        for ( unsigned int j=0; j < src[i].len && copied < len ; j++ ) {
            dbg().debug(CALL_INFO,3,DBG_MSK_PQS_Q,"copied=%lu rV=%lu rP=%lu\n",
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

    dbg().debug(CALL_INFO,2,DBG_MSK_PQS_POST_SHORT,"num postedShortRecvBuffers %lu\n",
                                        m_postedShortBuffers.size());
    m_nic->dmaRecv( -1, ShortMsgQ, buf->ioVec, callback );
}

void ProcessQueuesState::loopSendReq( std::vector<IoVec>& vec, int core, void* key )
{
    m_dbg.debug(CALL_INFO,2,DBG_MSK_PQS_LOOP,"dest core=%d key=%p\n",core,key);

    m_loopLink->send(0, new LoopBackEvent( vec, core, key ) );
}

void ProcessQueuesState::loopSendResp( int core, void* key )
{
    m_dbg.debug(CALL_INFO,2,DBG_MSK_PQS_LOOP,"dest core=%d key=%p\n",core,key);
    m_loopLink->send(0, new LoopBackEvent( core, key ) );
}

void ProcessQueuesState::loopHandler( Event* ev )
{
    LoopBackEvent* event = static_cast< LoopBackEvent* >(ev);
    m_dbg.debug(CALL_INFO,1,DBG_MSK_PQS_LOOP,"%s core=%d key=%p\n",
        event->vec.empty() ? "Response" : "Request", event->core, event->key);

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

    m_dbg.debug(CALL_INFO,1,DBG_MSK_PQS_CB,"execute callback\n");

    event->callback();
    delete e;
}
