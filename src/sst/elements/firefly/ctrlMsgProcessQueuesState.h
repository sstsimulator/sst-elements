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

#ifndef COMPONENTS_FIREFLY_CTRLMSGPROCESSQUEUESSTATE_H
#define COMPONENTS_FIREFLY_CTRLMSGPROCESSQUEUESSTATE_H

#include <sst/core/output.h>
#include "ctrlMsgXXX.h"
#include "heapAddrs.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

template< class T1 >
class ProcessQueuesState 
{
    static const unsigned long MaxPostedShortBuffers = 512;
    static const unsigned long MinPostedShortBuffers = 5;
    typedef typename T1::Callback Callback;
    typedef typename T1::Callback2 Callback2;

  public:
    ProcessQueuesState( int verbose, int mask, T1& obj ) : 
        m_obj( obj ),
        m_getKey( 0 ),
        m_rspKey( 0 ),
        m_needRecv( 0 ),
        m_numNicRequestedShortBuff(0),
        m_numRecvLooped(0),
        m_missedInt( false ),
		m_intCtx(NULL)
    {
        m_dbg.init("", verbose, mask, Output::STDOUT );
        char buffer[100];
        snprintf(buffer,100,"@t:%#x:%d:CtrlMsg::ProcessQueuesState::@p():@l ",
                            obj.nic().getNodeId(), obj.info()->worldRank());
        dbg().setPrefix(buffer);
    }

    void finish() {
        dbg().verbose(CALL_INFO,1,0,"pstdRcvQ=%lu recvdMsgQ=%lu loopResp=%lu %lu\n", 
             m_pstdRcvQ.size(), m_recvdMsgQ.size(), m_loopResp.size(), m_funcStack.size() );
    }

    ~ProcessQueuesState() 
    {
        while ( ! m_postedShortBuffers.empty() ) {
            delete m_postedShortBuffers.begin()->first;
            delete m_postedShortBuffers.begin()->second;
            m_postedShortBuffers.erase( m_postedShortBuffers.begin() );
        }
    }

    void enterInit( bool );
    void enterInit_1( uint64_t addr, size_t length );

    void enterSend( _CommReq*, uint64_t exitDelay = 0 );
    void enterRecv( _CommReq*, uint64_t exitDelay = 0 );
    void enterWait( WaitReq*, uint64_t exitDelay = 0 );
    void enterMakeProgress( uint64_t exitDelay = 0 );

    void needRecv( int, int, size_t );

    void loopHandler( int, std::vector<IoVec>&, void* );
    void loopHandler( int, void* );

  private:

    class Msg {
      public:
        Msg( MatchHdr* hdr ) : m_hdr( hdr ) {}
        virtual ~Msg() {}
        MatchHdr& hdr() { return *m_hdr; }
        std::vector<IoVec>& ioVec() { return m_ioVec; }

      protected:
        std::vector<IoVec> m_ioVec;

      private:
        MatchHdr* m_hdr;
    };

    class ShortRecvBuffer : public Msg {
      public:
        ShortRecvBuffer(size_t length, HeapAddrs& _heap  ) : Msg( &hdr ), heap(_heap) 
        { 
			if ( length ) {
            	ioVec.resize(2);    
			} else {
            	ioVec.resize(1);    
			}

            ioVec[0].len = sizeof(hdr);
            ioVec[0].addr.simVAddr = heap.alloc(ioVec[0].len);
            ioVec[0].addr.backing = &hdr;

            if ( length ) {
                buf.resize( length );
            	ioVec[1].len = length;
            	ioVec[1].addr.simVAddr = heap.alloc(length);;
            	ioVec[1].addr.backing = &buf[0];
            } else {
				assert(0);
			}

            ProcessQueuesState<T1>::Msg::m_ioVec.push_back( ioVec[1] ); 
        }
		~ShortRecvBuffer() {
			for ( unsigned i = 0; i < ioVec.size(); i++) {
				heap.free( ioVec[i].addr.simVAddr );
			}
		}

        MatchHdr                hdr;
        std::vector<IoVec>      ioVec; 
        std::vector<unsigned char> buf;
        HeapAddrs& 				heap;
    };

    struct LoopReq : public Msg {
        LoopReq(int _srcCore, std::vector<IoVec>& _vec, void* _key ) :
            Msg( (MatchHdr*)_vec[0].addr.backing ), 
            srcCore( _srcCore ), vec(_vec), key( _key) 
        {
            ProcessQueuesState<T1>::Msg::m_ioVec.push_back( vec[1] ); 
        }

        int srcCore;
        std::vector<IoVec>& vec;
        void* key;
    };

    struct LoopResp{
        LoopResp( int _srcCore, void* _key ) : srcCore(_srcCore), key(_key) {}
        int srcCore;
        void* key;
    };

    struct GetInfo {
        _CommReq*  req;
        CtrlHdr    hdr;
        nid_t      nid;
        uint32_t   tag;
        size_t     length;
    };

    class FuncCtxBase {
      public:
        FuncCtxBase( Callback ret = NULL ) : 
			callback(ret) {}

		virtual ~FuncCtxBase() {}
        Callback getCallback() {
			return callback;
		} 

		void setCallback( Callback arg ) {
			callback = arg;
		}

	  private:
        Callback callback;
    };

    typedef std::deque<FuncCtxBase*> Stack; 

    class ProcessQueuesCtx : public FuncCtxBase {
	  public:
		ProcessQueuesCtx( Callback callback ) :
			FuncCtxBase( callback ) {}
    };
	
    class InterruptCtx : public FuncCtxBase {
	  public:
		InterruptCtx( Callback callback ) :
			FuncCtxBase( callback ) {}
    };

    class ProcessShortListCtx : public FuncCtxBase {
      public:
        ProcessShortListCtx( std::deque<Msg*>& q ) : 
            m_msgQ(q), m_iter(m_msgQ.begin()) { }

        MatchHdr&   hdr() {
            return (*m_iter)->hdr();
        }

        std::vector<IoVec>& ioVec() { 
     	 	return (*m_iter)->ioVec();
        }

        Msg* msg() {
            return *m_iter;
        }
        std::deque<Msg*>& getMsgQ() { return m_msgQ; }
        bool msgQempty() { return m_msgQ.empty(); } 

        _CommReq*    req; 

        void removeMsg() { 
            delete *m_iter;
            m_iter = m_msgQ.erase(m_iter);
        }

        bool isDone() { return m_iter == m_msgQ.end(); }
        void incPos() { ++m_iter; }
      private:
        std::deque<Msg*> 			m_msgQ; 
        typename std::deque<Msg*>::iterator 	m_iter;
    };

    class WaitCtx : public FuncCtxBase {
      public:
		WaitCtx( WaitReq* _req, Callback callback ) :
			FuncCtxBase( callback ), req( _req ) {}

        ~WaitCtx() { delete req; }

        WaitReq*    req;
    };

    class ProcessLongGetFiniCtx : public FuncCtxBase {
      public:
		ProcessLongGetFiniCtx( _CommReq* _req ) : req( _req ) {}

        ~ProcessLongGetFiniCtx() { }

        _CommReq*    req;
    };


    nid_t calcNid( _CommReq* req, MP::RankID rank ) {
        return obj().info()->getGroup( req->getGroup() )->getMapping( rank );
    }

    MP::RankID getMyRank( _CommReq* req ) {
        return obj().info()->getGroup( req->getGroup() )->getMyRank();
    } 


    void processLoopResp( LoopResp* );
    void processLongAck( GetInfo* );
    void processLongGetFini( Stack*, _CommReq* );
    void processLongGetFini0( Stack* );
    void processPioSendFini( _CommReq* );

    void processSend_0( _CommReq* );
    void processSend_1( _CommReq* );
    void processSend_2( _CommReq* );
    void processSendLoop( _CommReq* );

    void processRecv_0( _CommReq* );
    void processRecv_1( _CommReq* );

    void processMakeProgress( Stack* );

    void processWait_0( Stack* );
    void processWaitCtx_0( WaitCtx*, _CommReq* req );
    void processWaitCtx_1( WaitCtx*, _CommReq* req );
    void processWaitCtx_2( WaitCtx* );

    void processQueues( Stack* );
    void processQueues0( Stack* );

    void processShortList_0( Stack* );
    void processShortList_1( Stack* );
    void processShortList_2( Stack* );
    void processShortList_3( Stack* );
    void processShortList_4( Stack* );
    void processShortList_5( Stack* );

	void enableInt( FuncCtxBase*, void (ProcessQueuesState::*)( Stack*) );

    void foo();
    void foo0( Stack* );

    void copyIoVec( std::vector<IoVec>& dst, std::vector<IoVec>& src, size_t);

    Output& dbg()   { return m_dbg; }
    T1& obj()       { return m_obj; }

    void postShortRecvBuffer();

    void pioSendFiniVoid( void*, uint64_t );
    void pioSendFiniCtrlHdr( CtrlHdr*, uint64_t );
    void getFini( _CommReq* );
    void dmaRecvFiniGI( GetInfo*, uint64_t, nid_t, uint32_t, size_t );
    void dmaRecvFiniSRB( ShortRecvBuffer*, nid_t, uint32_t, size_t );

    bool        checkMatchHdr( MatchHdr& hdr, MatchHdr& wantHdr,
                                                    uint64_t ignore );
    _CommReq*	searchPostedRecv( MatchHdr& hdr, int& delay );
    void        print( char* buf, int len );

    void exit( int delay = 0 ) {
        dbg().verbose(CALL_INFO,2,1,"exit ProcessQueuesState\n"); 
        obj().passCtrlToFunction( m_exitDelay + delay );
        m_exitDelay = 0;
    }

    key_t    genGetKey() {
        key_t tmp = m_getKey;;
        ++m_getKey;
        if ( m_getKey == LongGetKey ) m_getKey = 0; 
        return tmp | LongGetKey;
    }  

    key_t    genRspKey() {
        key_t tmp = m_rspKey;
        ++m_rspKey;
        if ( m_rspKey == LongRspKey ) m_rspKey = 0; 
        return tmp | LongRspKey;
    }  

    key_t    genReadReqKey( region_t region ) {
        //assert ( region to big ); 
        return region | ReadReqKey;
    }  

    key_t    genReadRspKey( region_t region ) {
        //assert ( region to big ); 
        return region | ReadRspKey;
    }  

    T1&         m_obj;
    Output      m_dbg;

    key_t   m_getKey;
    key_t   m_rspKey;
    int     m_needRecv;
    int     m_numNicRequestedShortBuff;
    int     m_numRecvLooped;
    bool    m_missedInt;

    std::deque< _CommReq* >         m_pstdRcvQ;
    std::deque< Msg* >              m_recvdMsgQ;

    std::deque< _CommReq* >         m_longGetFiniQ;
    std::deque< GetInfo* >          m_longAckQ;

    Stack                           m_funcStack; 
    Stack                           m_intStack; 

    std::deque< LoopResp* >         m_loopResp;

    std::map< ShortRecvBuffer*, Callback2* > m_postedShortBuffers; 
	FuncCtxBase*	m_intCtx;
    uint64_t        m_exitDelay;

    HeapAddrs*      m_simVAddrs;
};

template< class T1 >
void ProcessQueuesState<T1>::enterInit( bool haveGlobalMemHeap )
{
    size_t length = 0;
    length += MaxPostedShortBuffers * (sizeof(MatchHdr) + 16 ) & ~15;
    length += MaxPostedShortBuffers * (obj().shortMsgLength() + 16 ) & ~15;
    length += MaxPostedShortBuffers * (sizeof(MatchHdr) + 16 ) & ~15;
    length += MaxPostedShortBuffers * (obj().shortMsgLength() + 16 ) & ~15;

    std::function<void(uint64_t)> callback = [=](uint64_t value){
		enterInit_1( value, length );      
        return 0;
    };

    if ( haveGlobalMemHeap ) {
        obj().memHeap().alloc( length, callback );
    } else {
        enterInit_1( 0x1000, length );
    }
}

template< class T1 >
void ProcessQueuesState<T1>::enterInit_1( uint64_t addr, size_t length )
{
    dbg().verbose(CALL_INFO,1,1,"simVAddr %" PRIx64 "  length=%zu\n", addr, length );

    m_simVAddrs = new HeapAddrs( addr, length );

    for ( unsigned long i = 0; i < MinPostedShortBuffers; i++ ) {
        postShortRecvBuffer();
    }

    obj().passCtrlToFunction( );
}

template< class T1 >
void ProcessQueuesState<T1>::enterSend( _CommReq* req, uint64_t exitDelay )
{
    m_exitDelay = exitDelay;
    req->setSrcRank( getMyRank( req ) );
    dbg().verbose(CALL_INFO,1,1,"req=%p$ delay=%" PRIu64 "\n", req, exitDelay );

    uint64_t delay = obj().txDelay( req->getLength() );

    Callback callback;
    if ( obj().nic().isLocal( calcNid( req, req->getDestRank() ) ) ) {
        callback =  std::bind( 
                    &ProcessQueuesState<T1>::processSendLoop, this, req );
    } else {
        callback = std::bind( 
                    &ProcessQueuesState<T1>::processSend_0, this, req );
    }
    obj().schedCallback( callback, delay);
}

template< class T1 >
void ProcessQueuesState<T1>::processSend_0( _CommReq* req )
{
    obj().memwrite( 
        std::bind( &ProcessQueuesState<T1>::processSend_1, this, req ),
        0, sizeof( req->hdr()) 
    ); 
}

template< class T1 >
void ProcessQueuesState<T1>::processSend_1( _CommReq* req )
{
    Callback callback = std::bind( 
        &ProcessQueuesState<T1>::processSend_2, this, req );

    size_t length = req->getLength( );

    if ( length > obj().shortMsgLength() ) {
        obj().mempin( callback, 0, length );
    } else {
        obj().memcpy( callback, 0, 1, length );
    }
}

template< class T1 >
void ProcessQueuesState<T1>::processSend_2( _CommReq* req )
{
    void* hdrPtr = NULL;
    size_t length = req->getLength( );

    IoVec hdrVec;   
    hdrVec.len = sizeof( req->hdr() ); 

    hdrVec.addr.simVAddr = m_simVAddrs->alloc( hdrVec.len );

    if ( length <= obj().shortMsgLength() ) {
		hdrVec.addr.backing = malloc( hdrVec.len );
        hdrPtr = hdrVec.addr.backing;
        memcpy( hdrVec.addr.backing, &req->hdr(), hdrVec.len );
    } else {
        hdrVec.addr.backing = &req->hdr(); 
    }

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );
    
    nid_t nid = calcNid( req, req->getDestRank() );

    if ( length <= obj().shortMsgLength() ) {
        dbg().verbose(CALL_INFO,1,1,"Short %lu bytes dest %#x\n",length,nid); 
        vec.insert( vec.begin() + 1, req->ioVec().begin(), 
                                        req->ioVec().end() );
        req->setDone( obj().sendReqFiniDelay( length ) );

    } else {
        dbg().verbose(CALL_INFO,1,1,"sending long message %lu bytes\n",length); 
        req->hdr().key = genGetKey();

        GetInfo* info = new GetInfo;


        info->req = req;

        std::vector<IoVec> hdrVec;
        hdrVec.resize(1);
        hdrVec[0].len = sizeof( info->hdr ); 
        hdrVec[0].addr.simVAddr = m_simVAddrs->alloc( hdrVec[0].len ); 
        hdrVec[0].addr.backing = &info->hdr; 
        
        Callback2* callback = new Callback2;
        *callback = std::bind( 
            &ProcessQueuesState<T1>::dmaRecvFiniGI, this, info, hdrVec[0].addr.simVAddr, 
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3
        );

        obj().nic().dmaRecv( nid, req->hdr().key, hdrVec, callback ); 
        obj().nic().regMem( nid, req->hdr().key, req->ioVec(), NULL );
    }

    Callback* callback = new Callback;
    *callback = std::bind( &ProcessQueuesState::pioSendFiniVoid, this, hdrPtr, hdrVec.addr.simVAddr );

    obj().nic().pioSend( nid, ShortMsgQ, vec, callback);

    if ( ! req->isBlocking() ) {
        exit();
    } else {
        enterWait( new WaitReq( req ), m_exitDelay );
    }
}

template< class T1 >
void ProcessQueuesState<T1>::processSendLoop( _CommReq* req )
{
    dbg().verbose(CALL_INFO,2,2,"key=%p\n", req);

    IoVec hdrVec;
    hdrVec.len = sizeof( req->hdr() );
    hdrVec.addr.simVAddr = 1; //m_simVAddrs->alloc( hdrVec.len );
    hdrVec.addr.backing = &req->hdr();

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );
    vec.insert( vec.begin() + 1, req->ioVec().begin(), 
                                        req->ioVec().end() );

    obj().loopSend( vec, obj().nic().calcCoreId( 
                        calcNid( req, req->getDestRank() ) ), req );

    if ( ! req->isBlocking() ) {
        exit();
    } else {
        enterWait( new WaitReq( req ), m_exitDelay );
    }
}

template< class T1 >
void ProcessQueuesState<T1>::enterRecv( _CommReq* req, uint64_t exitDelay )
{
    dbg().verbose(CALL_INFO,1,1,"req=%p$ delay=%" PRIu64 "\n", req, exitDelay );
    m_exitDelay = exitDelay;

    if ( m_postedShortBuffers.size() < MaxPostedShortBuffers ) {
        if ( m_numNicRequestedShortBuff ) {
            --m_numNicRequestedShortBuff; 
        } else if ( m_numRecvLooped ) {
            --m_numRecvLooped;
        } else {
            postShortRecvBuffer();
        }
    }

    m_pstdRcvQ.push_front( req );
    obj().statPstdRcv()->addData( m_pstdRcvQ.size() );

    size_t length = req->getLength( );

    Callback callback;

    if ( length > obj().shortMsgLength() ) {
        callback = std::bind(
                &ProcessQueuesState<T1>::processRecv_0, this, req ); 
    } else {
        callback = std::bind(
                &ProcessQueuesState<T1>::processRecv_1, this, req );
    }

    obj().schedCallback( callback, obj().rxPostDelay_ns( length ) );
}

template< class T1 >
void ProcessQueuesState<T1>::processRecv_0( _CommReq* req )
{
    dbg().verbose(CALL_INFO,1,1,"\n");

    obj().mempin(
        std::bind( &ProcessQueuesState<T1>::processRecv_1, this, req ),
        0, req->getLength() 
    );
}

template< class T1 >
void ProcessQueuesState<T1>::processRecv_1( _CommReq* req )
{
    if ( ! req->isBlocking() ) {
        exit();
    } else {
        enterWait( new WaitReq( req ), m_exitDelay );
    }
}

template< class T1 >
void ProcessQueuesState<T1>::enterMakeProgress(  uint64_t exitDelay  )
{
    dbg().verbose(CALL_INFO,1,1,"num pstd %lu, num short %lu\n",
                            m_pstdRcvQ.size(), m_recvdMsgQ.size() );

    m_exitDelay = exitDelay;

    WaitCtx* ctx = new WaitCtx ( NULL,
        std::bind( &ProcessQueuesState<T1>::processMakeProgress, this, &m_funcStack ) 
    );

    m_funcStack.push_back( ctx );

    processQueues( &m_funcStack );
}

template< class T1 >
void ProcessQueuesState<T1>::processMakeProgress( Stack* stack )
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

template< class T1 >
void ProcessQueuesState<T1>::enterWait( WaitReq* req, uint64_t exitDelay  )
{
    dbg().verbose(CALL_INFO,1,1,"num pstd %lu, num short %lu\n",
                            m_pstdRcvQ.size(), m_recvdMsgQ.size() );

    m_exitDelay = exitDelay;

    WaitCtx* ctx = new WaitCtx ( req,
        std::bind( &ProcessQueuesState<T1>::processWait_0, this, &m_funcStack ) 
    );

    m_funcStack.push_back( ctx );

    processQueues( &m_funcStack );
}

template< class T1 >
void ProcessQueuesState<T1>::processWait_0( Stack* stack )
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

template< class T1 >
void ProcessQueuesState<T1>::processWaitCtx_0( WaitCtx* ctx, _CommReq* req )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    obj().schedCallback( 
        std::bind( &ProcessQueuesState<T1>::processWaitCtx_1, this, ctx, req ),
        req->getFiniDelay()
    );
}

template< class T1 >
void ProcessQueuesState<T1>::processWaitCtx_1( WaitCtx* ctx, _CommReq* req )
{
    size_t   length = req->getLength(); 

    dbg().verbose(CALL_INFO,1,1,"\n");

    if ( length > obj().shortMsgLength() ) {
        // this is a hack, to get point to point latencies to match Chama
        // we need to not add delay for unpinning the send buffer for long msg  
        if ( req->isSend() ) { 
            length = 0;
        } 
        obj().memunpin( 
            std::bind( &ProcessQueuesState<T1>::processWaitCtx_2, this, ctx ),
            0, length 
        ); 
    } else {
        processWaitCtx_2( ctx );
    }

    if ( req->isMine() ) {
        delete req;
    }
}

template< class T1 >
void ProcessQueuesState<T1>::processWaitCtx_2( WaitCtx* ctx )
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

template< class T1 >
void ProcessQueuesState<T1>::processQueues( Stack* stack )
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
                bind( &ProcessQueuesState<T1>::processQueues0, this, stack )  
        ); 
        stack->push_back( ctx );

        processLongGetFini( stack, m_longGetFiniQ.front() );

        m_longGetFiniQ.pop_front();

    } else if ( ! m_recvdMsgQ.empty() ) {

        ProcessQueuesCtx* ctx = new ProcessQueuesCtx(
                bind ( &ProcessQueuesState<T1>::processQueues0, this, stack ) 
        );  
        stack->push_back( ctx );
        processShortList_0( stack );

    } else {
        obj().schedCallback( stack->back()->getCallback() );
    }
}

template< class T1 >
void ProcessQueuesState<T1>::processQueues0( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu recvdMsgQ.size()=%lu\n", 
            stack->size(), m_recvdMsgQ.size() ); 
    
    delete stack->back();
    stack->pop_back();
    obj().schedCallback( stack->back()->getCallback() );
}

template< class T1 >
void ProcessQueuesState<T1>::processShortList_0( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu recvdMsgQ.size()=%lu\n", 
            stack->size(), m_recvdMsgQ.size() ); 

    ProcessShortListCtx* ctx = new ProcessShortListCtx( m_recvdMsgQ );
	m_recvdMsgQ.clear();
    stack->push_back( ctx );

    processShortList_1( stack );
}

template< class T1 >
void ProcessQueuesState<T1>::processShortList_1( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu recvdMsgQ.size()=%lu\n", 
                        stack->size(), m_recvdMsgQ.size() ); 

    ProcessShortListCtx* ctx = 
                        static_cast<ProcessShortListCtx*>( stack->back() );
    
    int count = 0;
    ctx->req = searchPostedRecv( ctx->hdr(), count );

    obj().memwalk( 
        std::bind( &ProcessQueuesState<T1>::processShortList_2, this, stack ),
        count
    ); 
}

template< class T1 >
void ProcessQueuesState<T1>::processShortList_2( Stack* stack )
{
    ProcessShortListCtx* ctx = 
                        static_cast<ProcessShortListCtx*>( stack->back() );
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu\n", stack->size()); 

    if ( ctx->req ) {
        obj().schedCallback( 
            std::bind( 
                    &ProcessQueuesState<T1>::processShortList_3, this, stack ),
            obj().rxDelay( ctx->hdr().count * ctx->hdr().dtypeSize )
        ); 
    } else {
        ctx->incPos();
        processShortList_5( stack );
    }
}

template< class T1 >
void ProcessQueuesState<T1>::processShortList_3( Stack* stack )
{
    dbg().verbose(CALL_INFO,2,1,"stack.size()=%lu\n", stack->size()); 

    ProcessShortListCtx* ctx = static_cast<ProcessShortListCtx*>(stack->back());

    _CommReq* req = ctx->req;

    req->setResp( ctx->hdr().tag, ctx->hdr().rank, ctx->hdr().count );

    size_t length = ctx->hdr().count * ctx->hdr().dtypeSize;

    if ( length <= obj().shortMsgLength() || 
                            dynamic_cast<LoopReq*>( ctx->msg() ) ) 
    {
        dbg().verbose(CALL_INFO,2,1,"copyIoVec() short|loop message\n");

        copyIoVec( req->ioVec(), ctx->ioVec(), length );

        obj().memcpy( 
            std::bind(
                    &ProcessQueuesState<T1>::processShortList_4, this, stack ),
                1, 0, length 
        );

    } else {
        processShortList_4( stack );
    }
}

template< class T1 >
void ProcessQueuesState<T1>::processShortList_4( Stack* stack )
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
        obj().loopSend( loopReq->srcCore , loopReq->key );

    } else if ( length <= obj().shortMsgLength() ) { 
        dbg().verbose(CALL_INFO,1,1,"short\n");
        req->setDone( obj().recvReqFiniDelay( length ) );
    } else {

        dbg().verbose(CALL_INFO,1,1,"long\n");
        Callback* callback = new Callback;
        *callback = std::bind( &ProcessQueuesState<T1>::getFini,this, req );

        nid_t nid = calcNid( ctx->req, ctx->hdr().rank );

        obj().nic().get( nid, ctx->hdr().key, req->ioVec(), callback );

        req->m_ackKey = ctx->hdr().key; 
        req->m_ackNid = nid;
    }

    ctx->removeMsg();
    processShortList_5( stack );
}

template< class T1 >
void ProcessQueuesState<T1>::processShortList_5( Stack* stack )
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
        obj().schedCallback( stack->back()->getCallback() );
    } else {
        dbg().verbose(CALL_INFO,1,1,"work on next Msg\n");

        processShortList_1( stack );
    }
}

template< class T1 >
void ProcessQueuesState<T1>::processLoopResp( LoopResp* resp )
{
    dbg().verbose(CALL_INFO,1,2,"srcCore=%d\n",resp->srcCore );
    _CommReq* req = (_CommReq*)resp->key;  
    req->setDone();
    delete resp;
}

template< class T1 >
void ProcessQueuesState<T1>::getFini( _CommReq* req )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    m_longGetFiniQ.push_back( req );

    foo();
}

template< class T1 >
void ProcessQueuesState<T1>::dmaRecvFiniGI( GetInfo* info, uint64_t simVAddr, nid_t nid, 
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

    foo();
}

template< class T1 >
void ProcessQueuesState<T1>::dmaRecvFiniSRB( ShortRecvBuffer* buf, nid_t nid,
                                            uint32_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,1,"ShortMsgQ nid=%#x tag=%#x length=%lu\n",
                                                    nid, tag, length );
    dbg().verbose(CALL_INFO,1,1,"ShortMsgQ rank=%d tag=%#" PRIx64 " count=%d "
                            "dtypeSize=%d\n", buf->hdr.rank, buf->hdr.tag,
                            buf->hdr.count, buf->hdr.dtypeSize );

    assert( tag == (uint32_t) ShortMsgQ );
    m_recvdMsgQ.push_back( buf );
    obj().statRcvdMsg()->addData( m_recvdMsgQ.size() );

    foo();
    m_postedShortBuffers.erase(buf);
    dbg().verbose(CALL_INFO,1,1,"num postedShortRecvBuffers %lu\n",
                                        m_postedShortBuffers.size());
}

template< class T1 >
void ProcessQueuesState<T1>::enableInt( FuncCtxBase* ctx,
	void (ProcessQueuesState<T1>::*funcPtr)( Stack* ) )
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

	if ( m_missedInt ) foo();
}

template< class T1 >
void ProcessQueuesState<T1>::foo( )
{
	if ( ! m_intCtx || ! m_intStack.empty() ) {
    	dbg().verbose(CALL_INFO,2,1,"missed interrupt\n");
        m_missedInt = true;
		return;
	} 

    InterruptCtx* ctx = new InterruptCtx(
            std::bind( &ProcessQueuesState<T1>::foo0, this, &m_intStack ) 
    ); 

    m_intStack.push_back( ctx );

	processQueues( &m_intStack );
}

template< class T1 >
void ProcessQueuesState<T1>::foo0( Stack* stack )
{
    assert( 1 == stack->size() );
	delete stack->back();
	stack->pop_back();

    dbg().verbose(CALL_INFO,2,1,"\n" );

	Callback callback = m_intCtx->getCallback();

	m_intCtx = NULL;

    callback();

    if ( m_intCtx && m_missedInt ) {
        foo();
        m_missedInt = false;
    }
}

template< class T1 >
void ProcessQueuesState<T1>::pioSendFiniVoid( void* hdr, uint64_t simVAddr )
{
    dbg().verbose(CALL_INFO,1,1,"hdr=%p\n", hdr );
    if ( hdr ) {
        free( hdr );
    }
    m_simVAddrs->free( simVAddr );
}

template< class T1 >
void ProcessQueuesState<T1>::pioSendFiniCtrlHdr( CtrlHdr* hdr, uint64_t simVAddr )
{
    dbg().verbose(CALL_INFO,1,1,"MsgHdr, Ack sent key=%#x\n", hdr->key);
	m_simVAddrs->free(simVAddr);
    delete hdr;
    foo();
}

template< class T1 >
void ProcessQueuesState<T1>::processLongGetFini( Stack* stack, _CommReq* req )
{
    ProcessLongGetFiniCtx* ctx = new ProcessLongGetFiniCtx( req );
    dbg().verbose(CALL_INFO,1,1,"\n");

    stack->push_back( ctx );

    obj().schedCallback( 
        std::bind( &ProcessQueuesState<T1>::processLongGetFini0, this, stack),
        obj().sendAckDelay() 
    );
}

template< class T1 > 
void ProcessQueuesState<T1>::processLongGetFini0( Stack* stack )
{
    _CommReq* req = static_cast<ProcessLongGetFiniCtx*>(stack->back())->req;

    dbg().verbose(CALL_INFO,1,1,"\n");
    
    delete stack->back();
    stack->pop_back();

    req->setDone( obj().recvReqFiniDelay( req->getLength() ) );

    IoVec hdrVec;   
    CtrlHdr* hdr = new CtrlHdr;
    hdrVec.len = sizeof( *hdr ); 
    hdrVec.addr.simVAddr = m_simVAddrs->alloc(hdrVec.len);
    hdrVec.addr.backing = hdr; 

    Callback* callback = new Callback;
    *callback = std::bind( 
                &ProcessQueuesState<T1>::pioSendFiniCtrlHdr, this, hdr, hdrVec.addr.simVAddr ); 

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );

    dbg().verbose(CALL_INFO,1,1,"send long msg Ack to nid=%d key=%#x\n", 
                                                req->m_ackNid, req->m_ackKey );

    obj().nic().pioSend( req->m_ackNid, req->m_ackKey, vec, callback );

    obj().schedCallback( stack->back()->getCallback() );
}


template< class T1 >
void ProcessQueuesState<T1>::processLongAck( GetInfo* info )
{
    dbg().verbose(CALL_INFO,1,1,"acked\n");
    info->req->setDone( obj().sendReqFiniDelay( info->req->getLength() ) );
    delete info;
    return;
}

template< class T1 >
void ProcessQueuesState<T1>::needRecv( int nid, int tag, size_t length  )
{
    dbg().verbose(CALL_INFO,1,1,"nid=%d tag=%#x length=%lu\n",nid,tag,length);
    if ( tag != (uint32_t) ShortMsgQ ) {
        dbg().fatal(CALL_INFO,0,"nid=%d tag=%#x length=%lu\n",nid,tag,length);
    }

    ++m_needRecv;
    foo();
}

template< class T1 >
void ProcessQueuesState<T1>::loopHandler( int srcCore, void* key )
{
    dbg().verbose(CALL_INFO,1,2,"resp: srcCore=%d key=%p \n",srcCore,key);

    m_loopResp.push_back( new LoopResp( srcCore, key ) );

    foo();
}

template< class T1 >
void ProcessQueuesState<T1>::loopHandler( int srcCore, std::vector<IoVec>& vec, void* key )
{
        
    MatchHdr* hdr = (MatchHdr*) vec[0].addr.backing;

    dbg().verbose(CALL_INFO,1,2,"req: srcCore=%d key=%p vec.size()=%lu srcRank=%d\n",
                                                    srcCore, key, vec.size(), hdr->rank);

    ++m_numRecvLooped;
    m_recvdMsgQ.push_back( new LoopReq( srcCore, vec, key ) );
    obj().statRcvdMsg()->addData( m_recvdMsgQ.size() );

    foo();
}

template< class T1 >
_CommReq* ProcessQueuesState<T1>::searchPostedRecv( MatchHdr& hdr, int& count )
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

template< class T1 >
bool ProcessQueuesState<T1>::checkMatchHdr( MatchHdr& hdr, MatchHdr& wantHdr,
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

template< class T1 >
void ProcessQueuesState<T1>::copyIoVec( 
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

            if ( dst[rV].addr.backing && src[i].addr.backing ) {
                ((char*)dst[rV].addr.backing)[rP] = ((char*)src[i].addr.backing)[j];
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

template< class T1 >
void ProcessQueuesState<T1>::print( char* buf, int len )
{
    dbg().verbose(CALL_INFO,3,1,"addr=%p len=%d\n",buf,len);
    std::string tmp;
    for ( int i = 0; i < len; i++ ) {
        dbg().verbose(CALL_INFO,3,1,"%#03x\n",(unsigned char)buf[i]);
    }
}

template< class T1 >
void ProcessQueuesState<T1>::postShortRecvBuffer( )
{
    ShortRecvBuffer* buf =
            new ShortRecvBuffer( obj().shortMsgLength() + sizeof(MatchHdr), *m_simVAddrs );

    Callback2* callback = new Callback2;
    *callback = std::bind( 
        &ProcessQueuesState<T1>::dmaRecvFiniSRB, this, buf, 
        std::placeholders::_1,
        std::placeholders::_2, 
        std::placeholders::_3
    ); 

    // save this info so we can cleanup in the destructor 
    m_postedShortBuffers[buf ] = callback;

    dbg().verbose(CALL_INFO,1,1,"num postedShortRecvBuffers %lu\n",
                                        m_postedShortBuffers.size());
    obj().nic().dmaRecv( -1, ShortMsgQ, buf->ioVec, callback ); 
}

}
}
}

#endif
