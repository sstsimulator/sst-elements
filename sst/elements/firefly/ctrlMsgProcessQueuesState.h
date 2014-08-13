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

#ifndef COMPONENTS_FIREFLY_CTRLMSGPROCESSQUEUESSTATE_H
#define COMPONENTS_FIREFLY_CTRLMSGPROCESSQUEUESSTATE_H

#include <sst/core/output.h>
#include "ctrlMsgState.h"
#include "ctrlMsgXXX.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

template< class T1 >
class ProcessQueuesState : StateBase< T1 > 
{
    static const unsigned long MaxPostedShortBuffers = 1;

  public:
    ProcessQueuesState( int verbose, Output::output_location_t loc, T1& obj ) : 
        StateBase<T1>( verbose, loc, obj ),
        m_getKey( 0 ),
        m_rspKey( 0 ),
        m_needRecv( 0 ),
        m_missedInt( false ),
		m_intCtx(NULL)
    {
        char buffer[100];
        snprintf(buffer,100,"@t:%#x:%d:CtrlMsg::ProcessQueuesState::@p():@l ",
                            obj.info()->nodeId(), obj.info()->worldRank());
        dbg().setPrefix(buffer);
        postShortRecvBuffer();
    }

    ~ProcessQueuesState() 
    {
        while ( ! m_postedShortBuffers.empty() ) {
            delete m_postedShortBuffers.begin()->first;
            delete m_postedShortBuffers.begin()->second;
            m_postedShortBuffers.erase( m_postedShortBuffers.begin() );
        }
    }

    void enterSend( _CommReq*, FunctorBase_0<bool>* func = NULL );
    void enterRecv( _CommReq*, FunctorBase_0<bool>* func = NULL );
    void enterWait( WaitReq*, FunctorBase_0<bool>* func = NULL );
    void needRecv( int, int, size_t );

    void loopHandler( int, std::vector<IoVec>&, void* );
    void loopHandler( int, void* );

	void send(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID dest, uint32_t tag,
        Hermes::Communicator group, FunctorBase_0<bool>* func ) 
    {
        dbg().verbose(CALL_INFO,1,0,"count=%d dest=%d tag=%#x\n",count,dest,tag);
        enterSend( new _CommReq( _CommReq::Send, buf, count, 
            obj().info()->sizeofDataType( dtype), dest, tag, group ), func );
    }

    void isend(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID dest, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageRequest* req,
        FunctorBase_0<bool>* func )
    {
        dbg().verbose(CALL_INFO,1,0,"count=%d dest=%d tag=%#x\n",count,dest,tag);
        *req = new _CommReq( _CommReq::Isend, buf, count, 
            obj().info()->sizeofDataType(dtype) , dest, tag, group );
        dbg().verbose(CALL_INFO,1,0,"%p\n",*req);
        enterSend( static_cast<_CommReq*>(*req), func );
    }

    void recv(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID src, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
        FunctorBase_0<bool>* func )
    {
        dbg().verbose(CALL_INFO,1,0,"count=%d src=%d tag=%#x\n",count,src,tag);
        enterRecv( new _CommReq( _CommReq::Recv, buf, count,
            obj().info()->sizeofDataType(dtype), src, tag, group, resp ), func); 
    }

    void irecv(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID src, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageRequest* req,
        FunctorBase_0<bool>* func )
    {
        dbg().verbose(CALL_INFO,1,0,"count=%d src=%d tag=%#x\n",count,src,tag);
        *req = new _CommReq( _CommReq::Irecv, buf, count, 
            obj().info()->sizeofDataType(dtype), src, tag, group ); 
        enterRecv( static_cast<_CommReq*>(*req), func );
    }

    void wait( Hermes::MessageRequest req, Hermes::MessageResponse* resp,
        FunctorBase_0<bool>* func )
    {
        dbg().verbose(CALL_INFO,1,0,"\n");
        
        enterWait( new WaitReq( req, resp ), func );
    }

    void waitAny( int count, Hermes::MessageRequest req[], int *index,
        Hermes::MessageResponse* resp, FunctorBase_0<bool>* func  ) 
    {
        dbg().verbose(CALL_INFO,1,0,"\n");
        enterWait( new WaitReq( count, req, index, resp ), func );
    }

    void waitAll( int count, Hermes::MessageRequest req[],
        Hermes::MessageResponse* resp[], FunctorBase_0<bool>* func )
    {
        dbg().verbose(CALL_INFO,1,0,"\n");
        enterWait( new WaitReq( count, req, resp ), func );
    }

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
        ShortRecvBuffer(size_t length ) : Msg( &hdr ) 
        { 
            ioVec.resize(2);    

            ioVec[0].ptr = &hdr;
            ioVec[0].len = sizeof(hdr);

            buf.resize( length );
            ioVec[1].ptr = &buf[0];
            ioVec[1].len = buf.size();

            ProcessQueuesState<T1>::Msg::m_ioVec.push_back( ioVec[1] ); 
        }

        MatchHdr                hdr;
        std::vector<IoVec>      ioVec; 
        std::vector<unsigned char> buf;
    };

    struct LoopReq : public Msg {
        LoopReq(int _srcCore, std::vector<IoVec>& _vec, void* _key ) :
            Msg( (MatchHdr*)_vec[0].ptr ), 
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
        bool       waitAck;
    };

    class FuncCtxBase {
      public:
        FuncCtxBase( FunctorBase_0< bool >* ret = NULL ) : 
			retFunctor(ret) {}

		virtual ~FuncCtxBase() {}
        FunctorBase_0< bool >* getRetFunctor() {
			return retFunctor;
		} 

		void setRetFunctor( FunctorBase_0< bool >* arg ) {
			retFunctor = arg;
		}

	  private:
        FunctorBase_0< bool >* retFunctor;
    };

    class ProcessQueuesCtx : public FuncCtxBase {
	  public:
		ProcessQueuesCtx( FunctorBase_0< bool >* ret) :
			FuncCtxBase( ret ) {}
    };
	
    class InterruptCtx : public FuncCtxBase {
	  public:
		InterruptCtx( FunctorBase_0< bool >* ret) :
			FuncCtxBase( ret ) {}
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
		WaitCtx( WaitReq* _req, FunctorBase_0< bool >* ret ) :
			FuncCtxBase( ret ), req( _req ) {}

        ~WaitCtx() { delete req; }

        WaitReq*    req;
    };

    nid_t calcNid( _CommReq* req, Hermes::RankID rank ) {
        return obj().info()->getGroup( req->getGroup() )->getNodeId(rank);
    }

    Hermes::RankID getMyRank( _CommReq* req ) {
        return obj().info()->getGroup( req->getGroup() )->getMyRank();
    } 


    void processLoopResp( LoopResp* );
    void processLongGet( GetInfo* );
    void processLongRsp( _CommReq* );
    void processPioSendFini( _CommReq* );

    bool enterSend( _CommReq* );
    bool enterSendLoop( _CommReq* );
    bool enterWait0( std::deque< FuncCtxBase* >& );
    void processQueues( std::deque< FuncCtxBase* >& );
    bool processQueues0( std::deque< FuncCtxBase* >& );
    void processShortList( std::deque< FuncCtxBase* >& );
    void processShortList0( std::deque< FuncCtxBase* >& );
    bool processShortList1( std::deque< FuncCtxBase* >& );
    bool processShortList2( std::deque< FuncCtxBase* >& );

	void enableInt( FuncCtxBase*, 
			bool (ProcessQueuesState::*)( std::deque< FuncCtxBase* >&) );

    void foo();
    bool foo0( std::deque< FuncCtxBase* >& );

    int copyIoVec( std::vector<IoVec>& dst, std::vector<IoVec>& src, size_t);

    Output& dbg()   { return StateBase<T1>::m_dbg; }
    T1& obj()       { return StateBase<T1>::obj; }

    void postShortRecvBuffer();

    template< class XX >
    struct SendFunctor :
        FunctorStatic_0< ProcessQueuesState, XX, bool > 
    { 
        SendFunctor( ProcessQueuesState* obj, 
                     bool (ProcessQueuesState::*fptr)(XX),
                     XX arg ) 
            : FunctorStatic_0< ProcessQueuesState,XX,bool >( obj,fptr,arg ) {}
    };

    template < class XX >
    SendFunctor<XX>* newPioSendFini( 
                        XX ptr, bool (ProcessQueuesState::*fptr)(XX) ) 
    {
        return new SendFunctor<XX>(this, fptr, ptr);
    };

    template< class XX >
    struct RecvFunctor :
        FunctorStatic_3< ProcessQueuesState, XX, nid_t, uint32_t, size_t, bool > 
    { 
        RecvFunctor( ProcessQueuesState* obj, 
        bool (ProcessQueuesState::*fptr)(XX, nid_t, uint32_t, size_t ), XX arg ) : 

        FunctorStatic_3< ProcessQueuesState, XX, nid_t, uint32_t, size_t, bool >
                (obj, fptr, arg) {}
    };

    template < class XX >
    RecvFunctor<XX>* newDmaRecvFini( XX ptr, 
                  bool (ProcessQueuesState::*fptr)(XX,nid_t,uint32_t,size_t) ) 
    {
        return new RecvFunctor<XX>(this, fptr, ptr);
    };

    bool pioSendFini( void* );
    bool pioSendFini( CtrlHdr* );
    bool dmaRecvFini( _CommReq*, nid_t, uint32_t, size_t );
    bool dmaRecvFini( GetInfo*, nid_t, uint32_t, size_t );
    bool dmaRecvFini( ShortRecvBuffer*, nid_t, uint32_t, size_t );

    bool        checkMatchHdr( MatchHdr& hdr, MatchHdr& wantHdr,
                                                    uint64_t ignore );
    _CommReq*	searchPostedRecv( MatchHdr& hdr, int& delay );
    void        print( char* buf, int len );

    void setExit( FunctorBase_0<bool>* exitFunctor ) {
        StateBase<T1>::setExit( exitFunctor );
    }

    FunctorBase_0<bool>* clearExit( ) {
        return StateBase<T1>::clearExit( );
    }

    void exit( int delay = 0 ) {
        StateBase<T1>::exit( delay );
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

    key_t   m_getKey;
    key_t   m_rspKey;
    int     m_needRecv;
    bool    m_missedInt;

    std::deque< _CommReq* >         m_pstdRcvQ;
    std::deque< Msg* >              m_recvdMsgQ;
    std::deque< _CommReq* >         m_longRspQ;
    std::deque< GetInfo* >          m_longGetQ;

    std::deque< FuncCtxBase* >      m_funcStack; 
    std::deque< FuncCtxBase* >      m_intStack; 

    std::deque< LoopResp* >         m_loopResp;

    std::map< ShortRecvBuffer*, RecvFunctor<ShortRecvBuffer*> * >
                                                    m_postedShortBuffers; 
	FuncCtxBase*	m_intCtx;

};

template< class T1 >
void ProcessQueuesState<T1>::enterSend( _CommReq* req,
                                        FunctorBase_0<bool>* exitFunctor )
{
    int delay = obj().txDelay();
    dbg().verbose(CALL_INFO,2,0,"new send CommReq\n");

    req->setSrcRank( getMyRank( req ) );

    FunctorStatic_0< ProcessQueuesState, _CommReq*, bool >*  functor;
    if ( obj().nic().isLocal( calcNid( req, req->getDestRank() ) ) ) {
        functor = new FunctorStatic_0< ProcessQueuesState, _CommReq*, bool > 
          ( this, &ProcessQueuesState::enterSendLoop, req );  
    } else {
        delay += obj().txNicDelay();
        size_t length = req->getLength( );

        if ( length > obj().shortMsgLength() ) {
            delay += obj().regRegionDelay( length );
        } else {
            delay += obj().memcpyDelay( length );
        }
        functor = new FunctorStatic_0< ProcessQueuesState, _CommReq*, bool > 
          ( this, &ProcessQueuesState::enterSend, req );  
    }

    setExit( exitFunctor );

    obj().schedFunctor( functor, delay );
}

template< class T1 >
bool ProcessQueuesState<T1>::enterSendLoop( _CommReq* req )
{
    dbg().verbose(CALL_INFO,2,0,"looop\n");

    IoVec hdrVec;
    hdrVec.ptr = &req->hdr();
    hdrVec.len = sizeof( req->hdr() );

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );
    vec.insert( vec.begin() + 1, req->ioVec().begin(), 
                                        req->ioVec().end() );

    obj().loopSend( vec, obj().nic().calcCoreId( 
                        calcNid( req, req->getDestRank() ) ), req );

    if ( ! req->isBlocking() ) {
        exit();
    } else {
        enterWait( new WaitReq( req ), clearExit() );
    }
    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::enterSend( _CommReq* req )
{
    void* hdrPtr = NULL;
    size_t length = req->getLength( );

    IoVec hdrVec;   
    hdrVec.len = sizeof( req->hdr() ); 

    if ( length <= obj().shortMsgLength() ) {
        hdrPtr = hdrVec.ptr = malloc( hdrVec.len );
        memcpy( hdrVec.ptr, &req->hdr(), hdrVec.len );
    } else {
        hdrVec.ptr = &req->hdr(); 
    }

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );
    
    nid_t nid = calcNid( req, req->getDestRank() );

    if ( length <= obj().shortMsgLength() ) {
        dbg().verbose(CALL_INFO,1,0,"Short %lu bytes dest %#x\n",length,nid); 
        vec.insert( vec.begin() + 1, req->ioVec().begin(), 
                                        req->ioVec().end() );
        req->setDone(obj().sendReqFiniDelay());

    } else {
        dbg().verbose(CALL_INFO,1,0,"sending long message %lu bytes\n",length); 
        req->hdr().key = genGetKey();

        GetInfo* info = new GetInfo;

        RecvFunctor<GetInfo*>* functor =
            newDmaRecvFini<GetInfo*>( info, &ProcessQueuesState::dmaRecvFini );

        info->waitAck = false;
        info->req = req;

        std::vector<IoVec> hdrVec;
        hdrVec.resize(1);
        hdrVec[0].ptr = &info->hdr; 
        hdrVec[0].len = sizeof( info->hdr ); 
        
        dbg().verbose(CALL_INFO,1,0,"post ack buffer, nid=%d get key=%#x\n",
                                        nid, req->hdr().key );
        obj().nic().dmaRecv( nid, req->hdr().key, hdrVec, functor ); 
        req->m_ackKey = req->hdr().key;
        req->m_ackNid = nid;
    }

    obj().nic().pioSend( nid, ShortMsgQ, vec,
          newPioSendFini<void*>( hdrPtr, &ProcessQueuesState::pioSendFini ) );

    if ( ! req->isBlocking() ) {
        exit();
    } else {
        enterWait( new WaitReq( req ), clearExit() );
    }

    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::enterRecv( _CommReq* req,
                                        FunctorBase_0<bool>* exitFunctor )
{
    dbg().verbose(CALL_INFO,1,0,"new recv CommReq\n");

    if ( m_postedShortBuffers.size() < MaxPostedShortBuffers ) {
        postShortRecvBuffer();
    }

    m_pstdRcvQ.push_front( req );

    if ( req->isBlocking() ) {
        enterWait( new WaitReq( req ), exitFunctor );
    } else {
        setExit( exitFunctor );
        exit();
    }
}

template< class T1 >
void ProcessQueuesState<T1>::enterWait( WaitReq* req, 
                                    FunctorBase_0<bool>* exitFunctor  )
{
    dbg().verbose(CALL_INFO,1,0,"num pstd %lu, num short %lu\n",
                            m_pstdRcvQ.size(), m_recvdMsgQ.size() );

    setExit( exitFunctor );

    WaitCtx* ctx = new WaitCtx ( req,
		new FunctorStatic_0< ProcessQueuesState,
                                    std::deque< FuncCtxBase* >&, bool > 
          		( this, &ProcessQueuesState::enterWait0, m_funcStack ) );

    m_funcStack.push_back( ctx );

    processQueues( m_funcStack );
}

template< class T1 >
bool ProcessQueuesState<T1>::enterWait0( std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,2,0,"stack.size()=%lu\n", stack.size()); 
    dbg().verbose(CALL_INFO,1,0,"num pstd %lu, num short %lu\n",
                            m_pstdRcvQ.size(), m_recvdMsgQ.size() );
    WaitCtx* ctx = static_cast<WaitCtx*>( stack.back() );

    stack.pop_back();

	assert( stack.empty() );

    if ( ctx->req->isDone() ) {
        exit( ctx->req->getDelay() );
        delete ctx;
        ctx = NULL;
        dbg().verbose(CALL_INFO,2,0,"exit found CommReq\n"); 
    }

	if ( ctx ) {
		enableInt( ctx, &ProcessQueuesState::enterWait0 );
	}

  	return true;
}

template< class T1 >
void ProcessQueuesState<T1>::processQueues( std::deque< FuncCtxBase* >& stack )
{
    int delay = 0;
    dbg().verbose(CALL_INFO,2,0,"shortMsgV.size=%lu\n", m_recvdMsgQ.size() );
    dbg().verbose(CALL_INFO,2,0,"stack.size()=%lu\n", stack.size()); 

    while ( m_needRecv ) {
        postShortRecvBuffer();
        --m_needRecv;
    } 

    while ( ! m_longRspQ.empty() ) {
        processLongRsp( m_longRspQ.front() );
        m_longRspQ.pop_front();
    }

    while ( ! m_longGetQ.empty() ) {
        processLongGet( m_longGetQ.front() );
        m_longGetQ.pop_front();
    }

    if ( ! m_loopResp.empty() ) {
        processLoopResp( m_loopResp.front() );
        m_loopResp.pop_front(); 
    }

    if ( ! m_recvdMsgQ.empty() ) {

        ProcessQueuesCtx* ctx = new ProcessQueuesCtx(
        	new FunctorStatic_0< ProcessQueuesState,
                                      std::deque< FuncCtxBase* >&, bool > 
          ( this, &ProcessQueuesState::processQueues0, stack ) );  
        stack.push_back( ctx );
        processShortList( stack );

    } else {
        obj().schedFunctor( stack.back()->getRetFunctor(), delay );
    }
}

template< class T1 >
bool ProcessQueuesState<T1>::processQueues0( std::deque< FuncCtxBase* >& stack )
{
    dbg().verbose(CALL_INFO,2,0,"stack.size()=%lu\n", stack.size()); 
    
    delete stack.back();
    stack.pop_back();
    obj().schedFunctor( stack.back()->getRetFunctor() );

    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::processShortList( std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,2,0,"stack.size()=%lu\n", stack.size()); 

    ProcessShortListCtx* ctx = new ProcessShortListCtx( m_recvdMsgQ );
	m_recvdMsgQ.clear();
    stack.push_back( ctx );

    processShortList0( stack );
}

template< class T1 >
void ProcessQueuesState<T1>::processShortList0(std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,2,0,"stack.size()=%lu\n", stack.size()); 

    ProcessShortListCtx* ctx = static_cast<ProcessShortListCtx*>( stack.back());
    
    int delay;
    ctx->req = searchPostedRecv( ctx->hdr(), delay );

    if ( ctx->req ) {
        delay += obj().rxDelay();
		
        if ( ! obj().nic().isLocal( calcNid( ctx->req, ctx->hdr().rank ) ) ) {
            delay += obj().rxNicDelay();
        }
    }

    FunctorBase_0< bool >* functor = new FunctorStatic_0< ProcessQueuesState,
                                            std::deque< FuncCtxBase* >&,
                                            bool > 
          ( this, &ProcessQueuesState::processShortList1, stack );  
    obj().schedFunctor( functor, delay );
}

template< class T1 >
bool ProcessQueuesState<T1>::processShortList1(std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,2,0,"stack.size()=%lu\n", stack.size()); 

    ProcessShortListCtx* ctx = static_cast<ProcessShortListCtx*>(stack.back());
    int delay = 0;

    if ( ctx->req ) {
        _CommReq* req = ctx->req;

        req->setResp( ctx->hdr().tag, ctx->hdr().rank, ctx->hdr().count );

        size_t length = ctx->hdr().count * ctx->hdr().dtypeSize;

        if ( length <= obj().shortMsgLength() || 
            dynamic_cast<LoopReq*>( ctx->msg() ) ) {

            dbg().verbose(CALL_INFO,2,0,"receive short|loop message\n"); 
            delay = copyIoVec( req->ioVec(), ctx->ioVec(), length );
        } else {
            dbg().verbose(CALL_INFO,1,0,"receive long message\n"); 
            delay += obj().regRegionDelay( length );
        }
    }

    FunctorBase_0< bool >* functor = new FunctorStatic_0< ProcessQueuesState,
                                            std::deque< FuncCtxBase* >&,
                                            bool > 
                    ( this, &ProcessQueuesState::processShortList2, stack );  
    obj().schedFunctor( functor, delay );

    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::processShortList2(std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,2,0,"stack.size()=%lu\n", stack.size()); 
    ProcessShortListCtx* ctx = static_cast<ProcessShortListCtx*>(stack.back());

    if ( ctx->req ) {

        _CommReq* req = ctx->req;

        size_t length = ctx->hdr().count * ctx->hdr().dtypeSize;

        LoopReq* loopReq;
        if ( ( loopReq = dynamic_cast<LoopReq*>( ctx->msg() ) ) ) {
		    dbg().verbose(CALL_INFO,1,0,"loop\n");
            req->setDone();
            obj().loopSend( loopReq->srcCore , loopReq->key );

        } else if ( length <= obj().shortMsgLength() ) { 
		    dbg().verbose(CALL_INFO,1,0,"short\n");
            req->setDone();
        } else {

            key_t rspKey = genRspKey();

            dbg().verbose(CALL_INFO,1,0,"long\n");
            RecvFunctor<_CommReq*>* rfunctor =
                newDmaRecvFini<_CommReq*>(req,&ProcessQueuesState::dmaRecvFini);

            nid_t nid = calcNid( ctx->req, ctx->hdr().rank );
            obj().nic().dmaRecv( nid, rspKey, req->ioVec(), rfunctor ); 

            req->m_ackKey = ctx->hdr().key; 
            req->m_ackNid = nid;

            IoVec hdrVec;   
            CtrlHdr* hdr = new CtrlHdr;
            hdrVec.ptr = hdr; 
            hdrVec.len = sizeof( *hdr ); 

            SendFunctor<CtrlHdr*>* functor = newPioSendFini<CtrlHdr*>( 
                    hdr, &ProcessQueuesState::pioSendFini );

            std::vector<IoVec> vec;
            vec.insert( vec.begin(), hdrVec );

            dbg().verbose(CALL_INFO,1,0,"send long msg get req get key=%#x"
                " response key=%#x\n", ctx->hdr().key, rspKey );
            hdr->key = rspKey;

            obj().nic().pioSend( nid, ctx->hdr().key, vec, functor );
        }
        ctx->removeMsg();
    } else {
        ctx->incPos();
    }

    if ( ctx->isDone() ) {
        dbg().verbose(CALL_INFO,2,0,"return up the stack\n");

		if ( ! ctx->msgQempty() ) {
			m_recvdMsgQ.insert( m_recvdMsgQ.begin(), ctx->getMsgQ().begin(),
														ctx->getMsgQ().end() );
		}
        delete stack.back(); 
        stack.pop_back();
        obj().schedFunctor( stack.back()->getRetFunctor() );
    } else {
        dbg().verbose(CALL_INFO,1,0,"work on next Msg\n");

        processShortList0( stack );
    }
    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::processLoopResp( LoopResp* resp )
{
    dbg().verbose(CALL_INFO,1,0,"srcCore=%d\n",resp->srcCore );
    _CommReq* req = (_CommReq*)resp->key;  
    req->setDone();
    delete resp;
}

template< class T1 >
bool ProcessQueuesState<T1>::dmaRecvFini( _CommReq* req, nid_t nid,
                                            uint32_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,0,"\n");
    assert( ( tag & LongRspKey ) ==  LongRspKey );
    m_longRspQ.push_back( req );

    foo();

    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::dmaRecvFini( GetInfo* info, nid_t nid, 
                                            uint32_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,0,"nid=%d tag=%#x length=%lu\n",
                                                    nid, tag, length );
    assert( ( tag & LongGetKey ) == LongGetKey );
    info->nid = nid;
    info->tag = tag;
    info->length = length;
    m_longGetQ.push_back( info );

    foo();

    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::dmaRecvFini( ShortRecvBuffer* buf, nid_t nid,
                                            uint32_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,0,"ShortMsgQ nid=%#x tag=%#x length=%lu\n",
                                                    nid, tag, length );
    dbg().verbose(CALL_INFO,1,0,"ShortMsgQ rank=%d tag=%#lx count=%d "
                            "dtypeSize=%d\n", buf->hdr.rank, buf->hdr.tag,
                            buf->hdr.count, buf->hdr.dtypeSize );

    assert( tag == (uint32_t) ShortMsgQ );
    m_recvdMsgQ.push_back( buf );

    foo();

    m_postedShortBuffers.erase(buf);
    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::enableInt( FuncCtxBase* ctx,
	bool (ProcessQueuesState<T1>::*funcPtr)( std::deque< FuncCtxBase* >&) )
{
  	dbg().verbose(CALL_INFO,2,0,"ctx=%p\n",ctx);
	assert( m_funcStack.empty() );
    if ( m_intCtx ) {
  	    dbg().verbose(CALL_INFO,2,0,"already have a return ctx\n");
        return; 
    }

	m_funcStack.push_back(ctx);
	ctx->setRetFunctor( new FunctorStatic_0< ProcessQueuesState,
          std::deque< FuncCtxBase* >&, bool > ( this, funcPtr, m_funcStack ) );
	m_intCtx = ctx;

	if ( m_missedInt ) foo();
}

template< class T1 >
void ProcessQueuesState<T1>::foo( )
{
	if ( ! m_intCtx || ! m_intStack.empty() ) {
    	dbg().verbose(CALL_INFO,2,0,"missed interrupt\n");
        m_missedInt = true;
		return;
	} 

    InterruptCtx* ctx = new InterruptCtx (
    		new FunctorStatic_0< ProcessQueuesState,
                               std::deque< FuncCtxBase* >&, bool > 
          			( this, &ProcessQueuesState::foo0, m_intStack ) ); 

    m_intStack.push_back( ctx );

	processQueues( m_intStack );
}

template< class T1 >
bool ProcessQueuesState<T1>::foo0(std::deque<FuncCtxBase*>& stack )
{
    assert( 1 == stack.size() );
	delete stack.back();
	stack.pop_back();

    dbg().verbose(CALL_INFO,2,0,"\n" );

    if ( m_missedInt ) {
        foo();
        m_missedInt = false;
        return true;
    }

	FunctorBase_0< bool >* retFunctor = m_intCtx->getRetFunctor();

	m_intCtx = NULL;

	if ( (*retFunctor)() ) {
		delete retFunctor;
	}

    return true; 
}

template< class T1 >
bool ProcessQueuesState<T1>::pioSendFini( void* hdr )
{
    dbg().verbose(CALL_INFO,1,0,"hdr=%p\n", hdr );
    if ( hdr ) {
        free( hdr );
    }

    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::pioSendFini( CtrlHdr* hdr )
{
    dbg().verbose(CALL_INFO,1,0,"MsgHdr, Ack sent key=%#x\n", hdr->key);
    delete hdr;
    foo();

    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::processLongRsp( _CommReq* req )
{
    req->setDone();

    IoVec hdrVec;   
    CtrlHdr* hdr = new CtrlHdr;
    hdrVec.ptr = hdr; 
    hdrVec.len = sizeof( *hdr ); 

    SendFunctor<CtrlHdr*>* functor = newPioSendFini<CtrlHdr*>( 
                    hdr, &ProcessQueuesState::pioSendFini );

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );

    dbg().verbose(CALL_INFO,1,0,"send long msg response getKey=%#x\n", 
                                                req->m_ackKey );

    obj().nic().pioSend( req->m_ackNid, req->m_ackKey, vec, functor );
}

template< class T1 >
void ProcessQueuesState<T1>::processLongGet( GetInfo* info )
{
    if ( info->waitAck ) {
        dbg().verbose(CALL_INFO,1,0,"acked\n");
        info->req->setDone(obj().sendReqFiniDelay());
        delete info;
        return;
    }

    dbg().verbose(CALL_INFO,1,0,"response key %#x\n",info->hdr.key);

    info->waitAck = true;

    nid_t nid = calcNid( info->req, info->req->getDestRank()  );
    dbg().verbose(CALL_INFO,1,0,"send to nid=%#x\n", nid );
    obj().nic().dmaSend( nid, info->hdr.key, info->req->ioVec(), NULL );

    RecvFunctor<GetInfo*>* functor =
            newDmaRecvFini<GetInfo*>( info, &ProcessQueuesState::dmaRecvFini );

    std::vector<IoVec> hdrVec;
    hdrVec.resize(1);
    hdrVec[0].ptr = &info->hdr; 
    hdrVec[0].len = sizeof( info->hdr ); 

    obj().nic().dmaRecv( info->req->m_ackNid, info->req->m_ackKey, hdrVec, functor ); 
}

template< class T1 >
void ProcessQueuesState<T1>::needRecv( int nid, int tag, size_t length  )
{
    dbg().verbose(CALL_INFO,1,0,"nid=%d tag=%#x length=%lu\n",nid,tag,length);
    assert( tag == (uint32_t) ShortMsgQ );
    ++m_needRecv;
    foo();
}

template< class T1 >
void ProcessQueuesState<T1>::loopHandler( int srcCore, void* key )
{
    dbg().verbose(CALL_INFO,1,0,"key=%p\n",key);

    m_loopResp.push_back( new LoopResp( srcCore, key ) );

    foo();
}

template< class T1 >
void ProcessQueuesState<T1>::loopHandler( int srcCore, std::vector<IoVec>& vec, void* key )
{
    dbg().verbose(CALL_INFO,1,0,"key=%p vec.size()=%lu\n",key, vec.size());
        
    MatchHdr* hdr = (MatchHdr*) vec[0].ptr;

    dbg().verbose(CALL_INFO,1,0,"src rank %d\n",hdr->rank);
    
    m_recvdMsgQ.push_back( new LoopReq( srcCore, vec, key ) );

    foo();
}

template< class T1 >
_CommReq* ProcessQueuesState<T1>::searchPostedRecv( MatchHdr& hdr, int& delay )
{
    _CommReq* req = NULL;
    int count = 0;
    dbg().verbose(CALL_INFO,1,0,"posted size %lu\n",m_pstdRcvQ.size());

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
    dbg().verbose(CALL_INFO,2,0,"req=%p\n",req);
    delay = obj().matchDelay(count);
    return req;
}

template< class T1 >
bool ProcessQueuesState<T1>::checkMatchHdr( MatchHdr& hdr, MatchHdr& wantHdr,
                                    uint64_t ignore )
{
    dbg().verbose(CALL_INFO,1,0,"want tag %#lx %#lx\n", wantHdr.tag, hdr.tag );
    if ( ( AnyTag != wantHdr.tag ) && 
            ( ( wantHdr.tag & ~ignore) != ( hdr.tag & ~ignore ) ) ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,0,"want rank %d %d\n", wantHdr.rank, hdr.rank );
    if ( ( Hermes::AnySrc != wantHdr.rank ) && ( wantHdr.rank != hdr.rank ) ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,0,"want group %d %d\n", wantHdr.group,hdr.group);
    if ( wantHdr.group != hdr.group ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,0,"want count %d %d\n", wantHdr.count,
                                    hdr.count);
    if ( wantHdr.count !=  hdr.count ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,0,"want dtypeSize %d %d\n", wantHdr.dtypeSize,
                                    hdr.dtypeSize);
    if ( wantHdr.dtypeSize !=  hdr.dtypeSize ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,0,"matched\n");
    return true;
}

template< class T1 >
int ProcessQueuesState<T1>::copyIoVec( 
                std::vector<IoVec>& dst, std::vector<IoVec>& src, size_t len )
{
    dbg().verbose(CALL_INFO,1,0,"dst.size() %lu src.size() %lu len=%lu\n",
                                    dst.size(), src.size(), len );

    dbg().verbose(CALL_INFO,1,0,"dst.ptr) %p dst.len() %lu\n",
                                    dst[0].ptr, dst[0].len );
    dbg().verbose(CALL_INFO,1,0,"src.ptr) %p src.len() %lu\n",
                                    src[0].ptr, src[0].len );
    size_t copied = 0;
    size_t rV = 0,rP =0;
    for ( unsigned int i=0; i < src.size() && copied < len; i++ ) 
    {
        assert( rV < dst.size() );
        dbg().verbose(CALL_INFO,3,0,"vec[%d].len %lu\n", i, src[i].len);

        for ( unsigned int j=0; j < src[i].len && copied < len ; j++ ) {

            assert( rP < dst[rV].len );

            if ( dst[rV].ptr && src[i].ptr ) {
                ((char*)dst[rV].ptr)[rP] = ((char*)src[i].ptr)[j];
            }
            ++copied;
            ++rP;
            if ( rP == dst[rV].len ) {
                ++rV;
            } 
        }
    }
    return obj().memcpyDelay( copied );
}

template< class T1 >
void ProcessQueuesState<T1>::print( char* buf, int len )
{
    dbg().verbose(CALL_INFO,3,0,"addr=%p len=%d\n",buf,len);
    std::string tmp;
    for ( int i = 0; i < len; i++ ) {
        dbg().verbose(CALL_INFO,3,0,"%#03x\n",(unsigned char)buf[i]);
    }
}

template< class T1 >
void ProcessQueuesState<T1>::postShortRecvBuffer( )
{
    ShortRecvBuffer* buf =
            new ShortRecvBuffer( obj().shortMsgLength() + sizeof(MatchHdr));

    RecvFunctor<ShortRecvBuffer*>* functor =
     newDmaRecvFini<ShortRecvBuffer*>( buf, &ProcessQueuesState::dmaRecvFini );

    // save this info so we can cleanup in the destructor 
    m_postedShortBuffers[buf ] = functor;

    dbg().verbose(CALL_INFO,1,0,"post short recv buffer\n");
    obj().nic().dmaRecv( -1, ShortMsgQ, buf->ioVec, functor ); 
}

}
}
}

#endif
