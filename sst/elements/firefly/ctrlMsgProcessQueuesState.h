// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
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
  public:
    ProcessQueuesState( int verbose, Output::output_location_t loc, T1& obj ) : 
        StateBase<T1>( verbose, loc, obj ),
        m_ackKey( 0 ),
        m_rspKey( 0 ),
        m_isRead( false ),
        m_pendingEvents( false ),
        m_needRecv( 0 ),
        m_enableInt( false ),
        m_missedInt( false )
    {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:CtrlMsg::ProcessQueuesState::@p():@l ",
                            obj.info()->nodeId(), obj.info()->worldRank());
        dbg().setPrefix(buffer);
        postShortRecvBuffer();
    }
    void enterSend( _CommReq*, FunctorBase_0<bool>* func = NULL );
    void enterRecv( _CommReq*, FunctorBase_0<bool>* func = NULL );
    void enterWait( std::set<_CommReq*>& reqs,
                                    FunctorBase_0<bool>* func = NULL );
    void needRecv( int, int, size_t );

    void enterRead( nid_t nid, region_t region, void* buf, size_t len,
                                     FunctorBase_0<bool>* func = NULL );
    void registerRegion( region_t region, nid_t nid, void* buf, size_t len, RegionEventQ* );
    void unregisterRegion( region_t );
    RegionEventQ* createEventQueue();


  private:
    struct ReadInfo {
        ReadInfo( region_t _region, void* _buf, size_t _len ) : 
            region( _region), buf( _buf ), len( _len ), done(false) {} 

        nid_t nid;
        region_t region;
        struct Hdr {
            key_t key;
        } hdr;
        void* buf;
        size_t len;
        bool done;
        RegionEventQ* q;
    };

    struct FuncCtxBase {
        StateArgsBase*  args;
        LocalsBase*     locals; 
        FunctorBase_0< bool >* retFunctor;
    };

    class EnterWaitArgs : public StateArgsBase {
      public:
        std::set<_CommReq*> reqs;
    };

    class ProcessShortListLocals : public LocalsBase {
      public:
        ShortRecvBuffer *buf;
        _CommReq*        req; 
        std::deque< ShortRecvBuffer* >::iterator iter;
        int pos;
    };

    class WaitLocals : public LocalsBase {
      public:
        std::set<_CommReq*> reqs;
    };

    class ReadLocals : public LocalsBase {
      public:
        ReadInfo* info;
    };

    struct AckInfo {
        _CommReq*  req;
        MsgHdr     hdr;
        nid_t      nid;
        tag_t      tag;
        size_t     length;
    };

    void processLongAck( AckInfo* );

    bool enterSend( _CommReq* );
    bool enterRead0( std::deque< FuncCtxBase* >& );
    bool enterWait0( std::deque< FuncCtxBase* >& );
    void processQueues( std::deque< FuncCtxBase* >& );
    bool processQueues0( std::deque< FuncCtxBase* >& );
    bool processShortList( std::deque< FuncCtxBase* >& );
    bool processShortList0( std::deque< FuncCtxBase* >& );
    bool processShortList1( std::deque< FuncCtxBase* >& );
    bool processShortList2( std::deque< FuncCtxBase* >& );
    bool processShortList3( std::deque< FuncCtxBase* >& );

    void processReadReq( ReadInfo* );

    void foo();
    bool foo0( std::deque< FuncCtxBase* >& );

    int copyBuf2Req( _CommReq* req, ShortRecvBuffer* buf );

  private:
    Output& dbg()   { return StateBase<T1>::m_dbg; }
    T1& obj()       { return StateBase<T1>::obj; }

    void postShortRecvBuffer( ShortRecvBuffer* buf = NULL );

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
        FunctorStatic_3< ProcessQueuesState, XX, nid_t, tag_t, size_t, bool > 
    { 
        RecvFunctor( ProcessQueuesState* obj, 
        bool (ProcessQueuesState::*fptr)(XX, nid_t, tag_t, size_t ), XX arg ) : 

        FunctorStatic_3< ProcessQueuesState, XX, nid_t, tag_t, size_t, bool >
                (obj, fptr, arg) {}
    };

    template < class XX >
    RecvFunctor<XX>* newDmaRecvFini( XX ptr, 
                  bool (ProcessQueuesState::*fptr)(XX,nid_t,tag_t,size_t) ) 
    {
        return new RecvFunctor<XX>(this, fptr, ptr);
    };

    bool pioSendFini( ReadInfo* );
    bool pioSendFini( MsgHdr* );
    bool pioSendFini( _CommReq* );
    bool dmaRecvFini( _CommReq*, nid_t, tag_t, size_t );
    bool dmaRecvFini( AckInfo*, nid_t, tag_t, size_t );
    bool dmaRecvReadBodyFini( ReadInfo*, nid_t, tag_t, size_t );
    bool dmaRecvReadReqFini( ReadInfo*, nid_t, tag_t, size_t );
    bool dmaRecvFini( ShortRecvBuffer*, nid_t, tag_t, size_t );
    bool dmaSendFini( _CommReq* );
    bool dmaSendFini( ReadInfo* );

    bool                checkMsgHdr( MsgHdr& hdr, MsgHdr& wantHdr );
    _CommReq*           searchPostedRecv( MsgHdr& hdr, int& delay );
    void                print( char* buf, int len );

    void enter( FunctorBase_0<bool>* exitFunctor ) {
        StateBase<T1>::set( exitFunctor );
    }

    void exit() {
        StateBase<T1>::exit();
    }

    key_t    genAckKey() {
        key_t tmp = m_ackKey;;
        ++m_ackKey;
        if ( m_ackKey == LongAckKey ) m_ackKey = 0; 
        return tmp | LongAckKey;
    }  

    key_t    genRspKey() {
        key_t tmp = m_rspKey;
        ++m_rspKey;
        if ( m_rspKey == LongAckKey ) m_rspKey = 0; 
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

    key_t               m_ackKey;
    key_t               m_rspKey;

    std::deque< _CommReq* >         m_pstdRcvQ;
    std::deque< ShortRecvBuffer* >  m_shortMsgV;
    std::deque< _CommReq* >         m_longRspQ;
    std::deque< AckInfo* >          m_longAckQ;
    std::deque<_CommReq*>           m_sendDmaFiniQ;
    std::deque<ReadInfo*>           m_sendDmaReadFiniQ;
    std::deque<ReadInfo*>           m_readBodyFiniQ;
    std::deque<ReadInfo*>           m_readReqFiniQ;

    std::deque< FuncCtxBase* >      m_funcStack; 
    std::deque< FuncCtxBase* >      m_intStack; 

    bool m_isRead;
    bool m_pendingEvents;
    int  m_needRecv;
    bool m_enableInt;
    bool m_missedInt;
};


template< class T1 >
void ProcessQueuesState<T1>::enterRead( nid_t nid, region_t region, void* buf, size_t len,
                                     FunctorBase_0<bool>* exitFunctor )
{
    dbg().verbose(CALL_INFO,1,0,"nid=%d region=%d buf=%p len=%lu \n",
                    nid, region, buf, len );
    enter( exitFunctor );

    ReadInfo* info = new ReadInfo( region, buf, len ) ;

    IoVec hdrVec;   
    hdrVec.ptr = &info->hdr; 
    hdrVec.len = sizeof( info->hdr ); 
    info->hdr.key = genReadRspKey( region );

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );

    SendFunctor<ReadInfo*>* sfunctor = 
            newPioSendFini<ReadInfo*>( info, &ProcessQueuesState::pioSendFini );

    RecvFunctor<ReadInfo*>* rfunctor =
            newDmaRecvFini<ReadInfo*>( info, &ProcessQueuesState::dmaRecvReadBodyFini );

    std::vector<IoVec> dataVec;
    dataVec.resize(1);
    dataVec[0].ptr = buf; 
    dataVec[0].len = len; 
        
    dbg().verbose(CALL_INFO,1,0,"rspkey=%#x\n", info->hdr.key );

    obj().nic().dmaRecv( nid, info->hdr.key, dataVec, rfunctor ); 

    obj().nic().pioSend( nid, genReadReqKey( region), vec, sfunctor );
}

template< class T1 >
bool ProcessQueuesState<T1>::enterRead0( std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,1,0,"stack.size()=%lu\n", stack.size()); 
    FuncCtxBase* ctx = stack.back();
    ReadLocals* locals = static_cast< ReadLocals* >( ctx->locals );
    if ( locals->info->done ) {
        m_pendingEvents = false;
        delete locals->info;
        delete locals;       
        delete ctx;
        stack.pop_back();
        m_isRead = false;
        m_missedInt = false;
        m_enableInt = false;
        exit();
        return true;
    }
    m_enableInt = true;
    dbg().verbose(CALL_INFO,1,0,"waiting for interrupt handler\n" ); 
    return false;
}

template< class T1 >
RegionEventQ* ProcessQueuesState<T1>::createEventQueue()
{
    dbg().verbose(CALL_INFO,1,0,"\n");
    return new RegionEventQ;
}

template< class T1 >
void ProcessQueuesState<T1>::registerRegion( region_t region, nid_t nid, void* buf,
                                                                    size_t len, RegionEventQ* q )
{
    dbg().verbose(CALL_INFO,1,0,"uregion=%d nid=%d buf=%p len=%lu \n",
                    region, nid, buf, len );

    
    ReadInfo* info = new ReadInfo( region, buf, len ) ;

    info->q = q;
    IoVec hdrVec;   
    hdrVec.ptr = &info->hdr; 
    hdrVec.len = sizeof( info->hdr ); 

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );

    key_t key = genReadReqKey( region );

    dbg().verbose(CALL_INFO,1,0,"reqkeyy=%#x\n",key);

    RecvFunctor<ReadInfo*>* rfunctor =
            newDmaRecvFini<ReadInfo*>( 
                info, &ProcessQueuesState::dmaRecvReadReqFini );

    obj().nic().dmaRecv( nid, key, vec, rfunctor ); 
}
template< class T1 >
void ProcessQueuesState<T1>::unregisterRegion( region_t region )
{
    dbg().verbose(CALL_INFO,1,0,"region %d\n",region);
}

template< class T1 >
void ProcessQueuesState<T1>::enterSend( _CommReq* req,
                                        FunctorBase_0<bool>* exitFunctor )
{
    dbg().verbose(CALL_INFO,1,0,"new send CommReq\n");

    FunctorStatic_0< ProcessQueuesState, _CommReq*, bool >*  functor;
    functor = new FunctorStatic_0< ProcessQueuesState, _CommReq*, bool > 
          ( this, &ProcessQueuesState::enterSend, req );  


    enter( exitFunctor );

    obj().schedFunctor( functor, obj().txDelay() );
}

template< class T1 >
bool ProcessQueuesState<T1>::enterSend( _CommReq* req )
{
    SendFunctor<_CommReq*>* functor = 
            newPioSendFini<_CommReq*>( req, &ProcessQueuesState::pioSendFini );

    IoVec hdrVec;   
    hdrVec.ptr = &req->hdr(); 
    hdrVec.len = sizeof( req->hdr() ); 

    std::vector<IoVec> vec;
    vec.insert( vec.begin(), hdrVec );

    dbg().verbose(CALL_INFO,1,0,"message length %lu\n", req->hdr().length );

    if ( req->hdr().length <= obj().shortMsgLength() ) {
        dbg().verbose(CALL_INFO,1,0,"sending short message\n"); 
        vec.insert( vec.begin() + 1, req->ioVec().begin(), 
                                        req->ioVec().end() );
    } else {
        dbg().verbose(CALL_INFO,1,0,"sending long message\n"); 
        req->hdr().ackKey = genAckKey();

        AckInfo* info = new AckInfo;
        RecvFunctor<AckInfo*>* functor =
            newDmaRecvFini<AckInfo*>( info, &ProcessQueuesState::dmaRecvFini );

        info->req = req;

        std::vector<IoVec> hdrVec;
        hdrVec.resize(1);
        hdrVec[0].ptr = &info->hdr; 
        hdrVec[0].len = sizeof( info->hdr ); 
        
        dbg().verbose(CALL_INFO,1,0,"post ack buffer, nid=%d ackeKy=%#x\n",
                                        req->nid(), req->hdr().ackKey );
        obj().nic().dmaRecv( req->nid(), req->hdr().ackKey, hdrVec, functor ); 
    }

    dbg().verbose(CALL_INFO,1,0,"req=%p %d\n",req,req->nid());
    obj().nic().pioSend( req->nid(), ShortMsgQ, vec, functor );

    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::enterRecv( _CommReq* req,
                                        FunctorBase_0<bool>* exitFunctor )
{
    dbg().verbose(CALL_INFO,1,0,"new recv CommReq\n");

    enter( exitFunctor );

    postShortRecvBuffer();

    m_pstdRcvQ.push_front( req );

    exit();
    m_missedInt = false;
    m_enableInt = false;
}

template< class T1 >
void ProcessQueuesState<T1>::enterWait(std::set<_CommReq*>& reqs, 
                                    FunctorBase_0<bool>* exitFunctor )
{
    dbg().verbose(CALL_INFO,1,0,"num Req %lu, num pstd %lu, num short %lu\n",
                reqs.size(), m_pstdRcvQ.size(), m_shortMsgV.size() );

    enter( exitFunctor );

    FuncCtxBase* ctx = new FuncCtxBase;

    ctx->retFunctor = new FunctorStatic_0< ProcessQueuesState,
                                            std::deque< FuncCtxBase* >&,
                                            bool > 
          ( this, &ProcessQueuesState::enterWait0, m_funcStack );  

    WaitLocals* locals = new WaitLocals;
    locals->reqs = reqs;
    ctx->locals = locals;
    m_funcStack.push_back( ctx );

    processQueues( m_funcStack );
}

template< class T1 >
bool ProcessQueuesState<T1>::enterWait0( std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,1,0,"stack.size()=%lu\n", stack.size()); 
    FuncCtxBase* ctx = stack.back();
    WaitLocals* locals = static_cast<WaitLocals*>(ctx->locals);

    std::set<_CommReq* >::iterator iter = locals->reqs.begin(); 
    for ( ; iter != locals->reqs.end();  ++iter ) {
        if ( (*iter)->isDone() ) {
            exit();
            delete ctx->locals;
            delete ctx;
            stack.pop_back();
            dbg().verbose(CALL_INFO,1,0,"exit found CommReq, stack.size()=%lu\n",
                                        stack.size()); 
            m_missedInt = false;
            m_enableInt = false;
            return true;
        }
    }

    if ( m_pendingEvents ) {
        m_pendingEvents = false;
        exit();
        delete ctx->locals;
        delete ctx;
        stack.pop_back();
        dbg().verbose(CALL_INFO,1,0,"exit region evet, stack.size()=%lu\n",
                                        stack.size()); 
        m_missedInt = false;
        m_enableInt = false;
        return true;
    }

    m_enableInt = true; 
    dbg().verbose(CALL_INFO,1,0,"waiting for interrupt handler\n" ); 
    return false;
}

template< class T1 >
void ProcessQueuesState<T1>::processQueues( std::deque< FuncCtxBase* >& stack )
{
    dbg().verbose(CALL_INFO,1,0,"shortMsgV.size=%lu\n", m_shortMsgV.size() );
    int delay = 0;
    dbg().verbose(CALL_INFO,1,0,"stack.size()=%lu\n", stack.size()); 

    while ( m_needRecv ) {
        postShortRecvBuffer();
        --m_needRecv;
    } 

    while ( ! m_longRspQ.empty() ) {
        m_longRspQ.front()->setDone();
        m_longRspQ.pop_front();
    }
    while ( ! m_longAckQ.empty() ) {
        processLongAck( m_longAckQ.front() );
        m_longAckQ.pop_front();
    }
    while ( ! m_sendDmaFiniQ.empty() ) {
        m_sendDmaFiniQ.front()->setDone();
        m_sendDmaFiniQ.pop_front();
    }

    while ( ! m_readReqFiniQ.empty() ) {
        processReadReq( m_readReqFiniQ.front() );
        m_readReqFiniQ.pop_front();
    }

    while ( ! m_sendDmaReadFiniQ.empty() ) {
        ReadInfo* info = m_sendDmaReadFiniQ.front();
        info->q->push_back( info->region ); 
        m_pendingEvents = true;
        delete m_sendDmaReadFiniQ.front();
        m_sendDmaReadFiniQ.pop_front();
    }

    while ( ! m_readBodyFiniQ.empty() ) {
        m_readBodyFiniQ.front()->done = true;
        m_readBodyFiniQ.pop_front();
    }
      
    if ( ! m_shortMsgV.empty() ) {
        FuncCtxBase* ctx = new FuncCtxBase;
        ctx->retFunctor = new FunctorStatic_0< ProcessQueuesState,
                                            std::deque< FuncCtxBase* >&,
                                            bool > 
          ( this, &ProcessQueuesState::processQueues0, stack );  
        stack.push_back( ctx );
        processShortList( stack );
 
    } else {
        obj().schedFunctor( stack.back()->retFunctor, delay );
    }
}

template< class T1 >
bool ProcessQueuesState<T1>::processQueues0( std::deque< FuncCtxBase* >& stack )
{
    dbg().verbose(CALL_INFO,1,0,"stack.size()=%lu\n", stack.size()); 
    
    delete stack.back();
    stack.pop_back();

    obj().schedFunctor( stack.back()->retFunctor );
    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::processShortList( std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,1,0,"stack.size()=%lu\n", stack.size()); 

    FuncCtxBase* ctx = new FuncCtxBase;
    ProcessShortListLocals* locals = new ProcessShortListLocals;
    locals->iter = m_shortMsgV.begin();
    ctx->locals = locals; 
    locals->buf = (*locals->iter);
    stack.push_back( ctx );

    processShortList0( stack );
 
    return false;
}

template< class T1 >
bool ProcessQueuesState<T1>::processShortList0(std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,1,0,"stack.size()=%lu\n", stack.size()); 

    FuncCtxBase* ctx = stack.back();
    ProcessShortListLocals* locals = 
                static_cast<ProcessShortListLocals*>(ctx->locals);
    
    int delay;
    locals->req = searchPostedRecv( locals->buf->hdr, delay );

    if ( locals->req ) {
        delay += obj().rxDelay();
    }

    FunctorBase_0< bool >* functor = new FunctorStatic_0< ProcessQueuesState,
                                            std::deque< FuncCtxBase* >&,
                                            bool > 
          ( this, &ProcessQueuesState::processShortList1, stack );  
    obj().schedFunctor( functor, delay );
    return false;
}

template< class T1 >
bool ProcessQueuesState<T1>::processShortList1(std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,1,0,"stack.size()=%lu\n", stack.size()); 

    FuncCtxBase* ctx = stack.back();
    ProcessShortListLocals* locals = 
                static_cast<ProcessShortListLocals*>(ctx->locals);
    int delay = 0;

    if ( locals->req ) {
        _CommReq* req = locals->req;
        ShortRecvBuffer* buf = locals->buf;

        req->setStatus( buf->hdr.nid, buf->hdr.tag, 
                            buf->hdr.length );
        if ( buf->hdr.length <= obj().shortMsgLength() ) {

            dbg().verbose(CALL_INFO,1,0,"receive short message\n"); 
            delay = copyBuf2Req( req, buf );
        } else {
            dbg().verbose(CALL_INFO,1,0,"receive long message\n"); 
        }
    }

    FunctorBase_0< bool >* functor = new FunctorStatic_0< ProcessQueuesState,
                                            std::deque< FuncCtxBase* >&,
                                            bool > 
                    ( this, &ProcessQueuesState::processShortList2, stack );  
    obj().schedFunctor( functor, delay );

    return false;
}

template< class T1 >
bool ProcessQueuesState<T1>::processShortList2(std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,1,0,"stack.size()=%lu\n", stack.size()); 
    FuncCtxBase* ctx = stack.back();
    ProcessShortListLocals* locals = 
                static_cast<ProcessShortListLocals*>(ctx->locals);
    if ( locals->req ) {
        _CommReq* req = locals->req;
        ShortRecvBuffer* buf = locals->buf;

        locals->iter = m_shortMsgV.erase( locals->iter );

        if ( buf->hdr.length <= obj().shortMsgLength()  ) {
            req->setDone();
        } else {

            key_t rspKey = genRspKey();

            dbg().verbose(CALL_INFO,1,0,"long\n");
            RecvFunctor<_CommReq*>* rfunctor =
                newDmaRecvFini<_CommReq*>( req, &ProcessQueuesState::dmaRecvFini );

            obj().nic().dmaRecv( buf->hdr.nid, rspKey, req->ioVec(), rfunctor ); 

            IoVec hdrVec;   
            MsgHdr* hdr = new MsgHdr;
            hdrVec.ptr = hdr; 
            hdrVec.len = sizeof( *hdr ); 

            SendFunctor<MsgHdr*>* functor = newPioSendFini<MsgHdr*>( 
                    hdr, &ProcessQueuesState::pioSendFini );

            std::vector<IoVec> vec;
            vec.insert( vec.begin(), hdrVec );


            dbg().verbose(CALL_INFO,1,0,"send long msg response ackKey=%#x"
                " rspkey=%#x\n", buf->hdr.ackKey, rspKey );
            hdr->rspKey = rspKey;
            hdr->ackKey = buf->hdr.ackKey;

            obj().nic().pioSend( req->nid(), hdr->ackKey, vec, functor );
            locals->iter = m_shortMsgV.end();
        }
    } else {
        ++locals->iter;
    } 

    if ( locals->iter == m_shortMsgV.end() ) {
        dbg().verbose(CALL_INFO,1,0,"return up the stack\n");
        delete stack.back()->locals;
        delete stack.back(); 
        stack.pop_back();

        obj().schedFunctor( stack.back()->retFunctor );
    } else {
        dbg().verbose(CALL_INFO,1,0,"set next buffer\n");
        locals->buf = (*locals->iter);

        processShortList0( stack );
    }
    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::dmaRecvFini( _CommReq* req, nid_t nid,
                                            tag_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,0,"\n");
    assert( ( tag & LongRspKey ) ==  LongRspKey );
    m_longRspQ.push_back( req );

    foo();

    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::dmaRecvFini( AckInfo* info, nid_t nid, 
                                            tag_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,0,"nid=%d tag=%#x length=%lu\n",
                                                    nid, tag, length );
    assert( ( tag & LongAckKey ) == LongAckKey );
    info->nid = nid;
    info->tag = tag;
    info->length = length;
    m_longAckQ.push_back( info );

    foo();

    return true;
}
template< class T1 >
bool ProcessQueuesState<T1>::dmaRecvReadReqFini( ReadInfo* info, nid_t nid, 
                                            tag_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,0,"nid=%d tag=%#x length=%lu\n",
                                                    nid, tag, length );
    assert( ( tag & ReadReqKey ) == ReadReqKey );
    info->nid = nid;

    m_readReqFiniQ.push_back( info );
    
    foo();

    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::processReadReq( ReadInfo* info )
{
    dbg().verbose(CALL_INFO,1,0,"nid=%d key=%#x\n", info->nid,info->hdr.key);

    std::vector<IoVec> dataVec;
    dataVec.resize(1);
    dataVec[0].ptr = info->buf; 
    dataVec[0].len = info->len; 

    SendFunctor<ReadInfo*>* functor = 
        newPioSendFini<ReadInfo*>( info, &ProcessQueuesState::dmaSendFini );

    obj().nic().dmaSend( info->nid, info->hdr.key, dataVec, functor );
}

template< class T1 >
bool ProcessQueuesState<T1>::dmaRecvReadBodyFini( ReadInfo* info, nid_t nid, 
                                            tag_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,0,"nid=%d tag=%#x length=%lu\n",
                                                    nid, tag, length );

    assert( ( tag & ReadRspKey ) == ReadRspKey );

    m_readBodyFiniQ.push_back( info );
    
    foo();

    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::dmaRecvFini( ShortRecvBuffer* buf, nid_t nid,
                                            tag_t tag, size_t length )
{
    dbg().verbose(CALL_INFO,1,0,"ShortMsgQ nid=%d tag=%#x length=%lu\n",
                                                    nid, tag, length );
    assert( tag == (tag_t) ShortMsgQ );
    m_shortMsgV.push_back( buf );
    foo();

    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::foo( )
{
    dbg().verbose(CALL_INFO,1,0,"m_funcStack.size()=%lu m_enableInt=%s\n",
                           m_funcStack.size(), m_enableInt ? "true":"false" );
    if ( ! m_enableInt ) {
        dbg().verbose(CALL_INFO,1,0,"missed interrupt\n");
        m_missedInt = true;
        return;
    }
    m_enableInt = false;

    FuncCtxBase* ctx = new FuncCtxBase;

    ctx->retFunctor = new FunctorStatic_0< ProcessQueuesState,
                                            std::deque< FuncCtxBase* >&,
                                            bool > 
          ( this, &ProcessQueuesState::foo0, m_intStack );  

    m_intStack.push_back( ctx );

    processQueues( m_intStack );
}

template< class T1 >
bool ProcessQueuesState<T1>::foo0(std::deque<FuncCtxBase*>& stack )
{
    dbg().verbose(CALL_INFO,1,0,"m_intStack.size()=%lu\n", stack.size() );

    assert( ! stack.empty() );

    if ( m_isRead ) {
        enterRead0(m_funcStack);
    } else {
        enterWait0(m_funcStack);
    }
    if ( ! m_funcStack.empty() && m_missedInt ) {
        dbg().verbose(CALL_INFO,1,0,"clearing missed interrupt\n");
        m_missedInt = false;
        processQueues( stack );
        return false; 
    } else {
        delete stack.back(); 
        stack.pop_back();
        return true; 
    }
}

template< class T1 >
bool ProcessQueuesState<T1>::pioSendFini( MsgHdr* hdr )
{
    dbg().verbose(CALL_INFO,1,0,"MsgHdr, Ack sent ackKey=%#x rspKey=%#x\n",
                                    hdr->ackKey, hdr->rspKey);
    delete hdr;
    foo();

    return true;
}

template< class T1 >
bool ProcessQueuesState<T1>::pioSendFini( ReadInfo* info )
{
    dbg().verbose(CALL_INFO,1,0,"ReadInfo, key=%#x\n", 
                    genReadReqKey( info->region ) );

    FuncCtxBase* ctx = new FuncCtxBase;

    ctx->retFunctor = new FunctorStatic_0< ProcessQueuesState,
                                            std::deque< FuncCtxBase* >&,
                                            bool > 
          ( this, &ProcessQueuesState::enterRead0, m_funcStack );  

    ReadLocals* locals = new ReadLocals;
    locals->info = info;
    ctx->locals = locals;
    m_funcStack.push_back( ctx );
    m_isRead = true;

    processQueues( m_funcStack );

    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::processLongAck( AckInfo* info )
{
    dbg().verbose(CALL_INFO,1,0,"ackKey=%#x rspKey=%#x\n",info->hdr.ackKey,
                                        info->hdr.rspKey);

    SendFunctor<_CommReq*>* functor = 
    newPioSendFini<_CommReq*>( info->req, &ProcessQueuesState::dmaSendFini );

    dbg().verbose(CALL_INFO,1,0,"req=%p\n",info->req);
    dbg().verbose(CALL_INFO,1,0,"send to %d\n",info->req->nid() );
    obj().nic().dmaSend( info->req->nid(), info->hdr.rspKey,
                                                info->req->ioVec(), functor );
    delete info;
}


template< class T1 >
bool ProcessQueuesState<T1>::dmaSendFini( _CommReq* req )
{
    dbg().verbose(CALL_INFO,1,0,"_CommReq\n");
    m_sendDmaFiniQ.push_back( req );
    
    foo();
    
    return true; 
}

template< class T1 >
bool ProcessQueuesState<T1>::dmaSendFini( ReadInfo* info )
{
    dbg().verbose(CALL_INFO,1,0,"ReadInfo\n");
    m_sendDmaReadFiniQ.push_back( info );
    
    foo();
    
    return true; 
}

template< class T1 >
bool ProcessQueuesState<T1>::pioSendFini( _CommReq* req )
{
    dbg().verbose(CALL_INFO,1,0,"_CommReq\n");

    if ( req->hdr().length <= obj().shortMsgLength() ) {
        req->setDone();
    }

    exit();
    m_missedInt = false;
    m_enableInt = false;

    return true;
}

template< class T1 >
void ProcessQueuesState<T1>::needRecv( int nid, int tag, size_t length  )
{
    dbg().verbose(CALL_INFO,1,0,"nid=%d tag=%#x length=%lu\n",nid,tag,length);
    assert( tag == (tag_t) ShortMsgQ );
    ++m_needRecv;
    foo();
}

template< class T1 >
_CommReq* ProcessQueuesState<T1>::searchPostedRecv( MsgHdr& hdr, int& delay )
{
    _CommReq* req = NULL;
    int count = 0;
    dbg().verbose(CALL_INFO,1,0,"posted size %lu\n",m_pstdRcvQ.size());

    std::deque< _CommReq* >:: iterator iter = m_pstdRcvQ.begin();
    for ( ; iter != m_pstdRcvQ.end(); ++iter ) {

        ++count;
        if ( ! checkMsgHdr( hdr, (*iter)->hdr() ) ) {
            continue;
        }

        req = *iter;
        m_pstdRcvQ.erase(iter);
        break;
    }
    dbg().verbose(CALL_INFO,1,0,"req=%p\n",req);
    delay = obj().matchDelay(count);
    return req;
}

template< class T1 >
bool ProcessQueuesState<T1>::checkMsgHdr( MsgHdr& hdr, MsgHdr& wantHdr )
{
    dbg().verbose(CALL_INFO,1,0,"want tag %#x %#x\n", wantHdr.tag, hdr.tag );
    if ( ( AnyTag != wantHdr.tag ) && ( wantHdr.tag != hdr.tag ) ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,0,"want nid %d %d\n", wantHdr.nid, hdr.nid );
    if ( ( AnyNid != wantHdr.nid ) && ( wantHdr.nid != hdr.nid ) ) {
        return false;
    }

    dbg().verbose(CALL_INFO,1,0,"wantlen %lu %lu\n", wantHdr.length, hdr.length);
    if ( wantHdr.length < hdr.length ) {
        return false;
    }
    dbg().verbose(CALL_INFO,1,0,"matched\n");
    return true;
}

template< class T1 >
int ProcessQueuesState<T1>::copyBuf2Req( _CommReq* req, ShortRecvBuffer* buf )
{
    size_t offset = 0;

    dbg().verbose(CALL_INFO,1,0,"ioVec().size() %lu, length %lu\n",
                req->ioVec().size(), buf->hdr.length);

    std::vector<IoVec>& ioVec = req->ioVec();

    for ( unsigned int i=0; i < ioVec.size() && 
                                    offset < buf->hdr.length; i++ ) 
    {
        dbg().verbose(CALL_INFO,1,0,"vec[%d].len %lu\n", i, ioVec[i].len);

        size_t len = ioVec[i].len < buf->hdr.length - offset ?
                         ioVec[i].len : buf->hdr.length - offset;

        memcpy( ioVec[i].ptr, &buf->buf[offset], len );
        print( (char*)ioVec[i].ptr, len );
        offset += len;

    }
    return obj().memcpyDelay( offset );
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
void ProcessQueuesState<T1>::postShortRecvBuffer( ShortRecvBuffer* buf )
{
    if ( NULL == buf ) {
        buf = new ShortRecvBuffer( obj().shortMsgLength() + sizeof(MsgHdr));
    }

    RecvFunctor<ShortRecvBuffer*>* functor =
     newDmaRecvFini<ShortRecvBuffer*>( buf, &ProcessQueuesState::dmaRecvFini );

    dbg().verbose(CALL_INFO,1,0,"post short recv buffer\n");
    obj().nic().dmaRecv( -1, ShortMsgQ, buf->ioVec, functor ); 
}

}
}
}

#endif
