// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sstream>

#include "virtNic.h"
#include "ctrlMsgXXX.h"
#include "ctrlMsgSendState.h"
#include "ctrlMsgRecvState.h"
#include "ctrlMsgWaitAnyState.h"
#include "ctrlMsgWaitAllState.h"
#include "ctrlMsgProcessQueuesState.h"
#include "info.h"
#include "loopBack.h"

using namespace SST::Firefly::CtrlMsg;
using namespace SST::Firefly;
using namespace SST;

XXX::XXX( Component* owner, Params& params ) :
    m_rxPostMod( NULL ),
    m_retLink( NULL ),
    m_info( NULL ),
    m_sendState( NULL ),
    m_recvState( NULL ),
    m_waitAnyState( NULL ),
    m_waitAllState( NULL ),
    m_processQueuesState( NULL )
{
    m_dbg_level = params.find_integer("verboseLevel",0);
    m_dbg_mask = params.find_integer("verboseMask",-1);

    m_dbg.init("@t:CtrlMsg::@p():@l ", 
        m_dbg_level, 
        m_dbg_mask,
        Output::STDOUT );

    std::stringstream ss;
    ss << this;

    m_delayLink = owner->configureSelfLink( 
        "CtrlMsgSelfLink." + ss.str(), "1 ns",
        new Event::Handler<XXX>(this,&XXX::delayHandler));

    m_matchDelay_ns = params.find_integer( "matchDelay_ns", 1 );

    std::string tmpName = params.find_string("txMemcpyMod");
    Params tmpParams = params.find_prefix_params("txMemcpyModParams.");
    m_txMemcpyMod = dynamic_cast<LatencyMod*>( 
            owner->loadModule( tmpName, tmpParams ) );  
    assert( m_txMemcpyMod );
    
    tmpName = params.find_string("rxMemcpyMod");
    tmpParams = params.find_prefix_params("rxMemcpyModParams.");
    m_rxMemcpyMod = dynamic_cast<LatencyMod*>( 
            owner->loadModule( tmpName, tmpParams ) );  
    assert( m_rxMemcpyMod );

    tmpName = params.find_string("txSetupMod");
    tmpParams = params.find_prefix_params("txSetupModParams.");
    m_txSetupMod = dynamic_cast<LatencyMod*>( 
            owner->loadModule( tmpName, tmpParams ) );  
    assert( m_txSetupMod );
    
    tmpName = params.find_string("rxSetupMod");
    tmpParams = params.find_prefix_params("rxSetupModParams.");
    m_rxSetupMod = dynamic_cast<LatencyMod*>( 
            owner->loadModule( tmpName, tmpParams ) );  
    assert( m_rxSetupMod );

    tmpName = params.find_string("rxPostMod");
    if ( ! tmpName.empty() ) {
        tmpParams = params.find_prefix_params("rxPostModParams.");
        m_rxPostMod = dynamic_cast<LatencyMod*>( 
            owner->loadModule( tmpName, tmpParams ) );  
    }

    m_regRegionBaseDelay_ns = params.find_integer( "regRegionBaseDelay_ns", 0 );
    m_regRegionPerPageDelay_ns = params.find_integer( "regRegionPerPageDelay_ns", 0 );
    m_regRegionXoverLength = params.find_integer( "regRegionXoverLength", 4096 );
    
    m_shortMsgLength = params.find_integer( "shortMsgLength", 4096 );

    tmpName = params.find_string("txFiniMod","firefly.LatencyMod");
    tmpParams = params.find_prefix_params("txFiniModParams.");
    m_txFiniMod = dynamic_cast<LatencyMod*>( 
            owner->loadModule( tmpName, tmpParams ) );  
    assert( m_txFiniMod );
    
    tmpName = params.find_string("rxFiniMod","firefly.LatencyMod");
    tmpParams = params.find_prefix_params("rxFiniModParams.");
    m_rxFiniMod = dynamic_cast<LatencyMod*>( 
            owner->loadModule( tmpName, tmpParams ) );  
    assert( m_rxFiniMod );
    
    m_sendAckDelay = params.find_integer( "sendAckDelay_ns", 0 );

    m_sendStateDelay = params.find_integer( "sendStateDelay_ps", 0 );
    m_recvStateDelay = params.find_integer( "recvStateDelay_ps", 0 );
    m_waitallStateDelay = params.find_integer( "waitallStateDelay_ps", 0 );
    m_waitanyStateDelay = params.find_integer( "waitanyStateDelay_ps", 0 );

    m_loopLink = owner->configureLink(
			params.find_string("loopBackPortName", "loop"), "1 ns",
            new Event::Handler<XXX>(this,&XXX::loopHandler) );
    assert(m_loopLink);
}

void XXX::finish() { 
    m_processQueuesState->finish(); 
}

XXX::~XXX()
{
	if ( m_sendState ) delete m_sendState;
	if ( m_recvState ) delete m_recvState;
	if ( m_waitAnyState) delete m_waitAnyState;
	if ( m_waitAllState) delete m_waitAllState;
	if ( m_processQueuesState) delete m_processQueuesState;

    delete m_txMemcpyMod;
    delete m_rxMemcpyMod;
    delete m_txSetupMod;
    delete m_rxSetupMod;
    delete m_rxFiniMod;
    delete m_txFiniMod;

    if ( m_rxPostMod ) {
        delete m_rxPostMod;
    }
}

void XXX::init( Info* info, VirtNic* nic )
{
    m_info = info;
    m_nic = nic;
    nic->setNotifyOnGetDone(
        new VirtNic::Handler<XXX,void*>(this, &XXX::notifyGetDone )
    );
    nic->setNotifyOnSendPioDone(
        new VirtNic::Handler<XXX,void*>(this, &XXX::notifySendPioDone )
    );
    nic->setNotifyOnRecvDmaDone(
        new VirtNic::Handler4Args<XXX,int,int,size_t,void*>(
                                this, &XXX::notifyRecvDmaDone )
    );
    nic->setNotifyNeedRecv(
        new VirtNic::Handler3Args<XXX,int,int,size_t>(
                                this, &XXX::notifyNeedRecv )
    );
}

void XXX::setup() 
{
    char buffer[100];
    snprintf(buffer,100,"@t:%#x:%d:CtrlMsg::XXX::@p():@l ",m_nic->getNodeId(), 
                                                m_info->worldRank());
    m_dbg.setPrefix(buffer);

    m_sendState = new SendState<XXX>( m_dbg_level, m_dbg_mask, *this );
    m_recvState = new RecvState<XXX>( m_dbg_level, m_dbg_mask, *this );
    m_waitAnyState = new WaitAnyState<XXX>( m_dbg_level, m_dbg_mask, *this );
    m_waitAllState = new WaitAllState<XXX>( m_dbg_level, m_dbg_mask, *this );
    m_processQueuesState = new ProcessQueuesState<XXX>(
                        m_dbg_level, m_dbg_mask, *this );

    m_dbg.verbose(CALL_INFO,1,1,"matchDelay %d ns.\n",  m_matchDelay_ns );
}

void XXX::setRetLink( Link* link ) 
{
    m_retLink = link;
}

class Foo : public LoopBackEvent {
  public:
    Foo( std::vector<IoVec>& _vec, int core, void* _key ) : 
        LoopBackEvent( core ), vec( _vec ), key( _key ), response( false ) 
    {}

    Foo( int core, void* _key ) : 
        LoopBackEvent( core ), key( _key ), response( true ) 
    {}

    std::vector<IoVec>  vec;
    void*               key;
    bool                response;
};

static size_t calcLength( std::vector<IoVec>& ioVec )
{
    size_t len = 0;
    for ( size_t i = 0; i < ioVec.size(); i++ ) {
        len += ioVec[i].len;
    }
    return len;
}

void XXX::loopSend( std::vector<IoVec>& vec, int core, void* key ) 
{
    m_dbg.verbose(CALL_INFO,1,1,"dest core=%d key=%p\n",core,key);    
    
    m_loopLink->send(0, new Foo( vec, core, key ) );
}    

void XXX::loopSend( int core, void* key ) 
{
    m_dbg.verbose(CALL_INFO,1,1,"dest core=%d key=%p\n",core,key);    
    m_loopLink->send(0, new Foo( core, key ) );
}

void XXX::loopHandler( Event* ev )
{
    Foo* event = static_cast< Foo* >(ev);
    m_dbg.verbose(CALL_INFO,1,1,"%s key=%p\n",
        event->vec.empty() ? "Response" : "Request", event->key);    

    if ( event->vec.empty() ) {
        m_processQueuesState->loopHandler(event->core, event->key );
    } else { 
        m_processQueuesState->loopHandler(event->core, event->vec, event->key);
    }
    delete ev;
}

void XXX::sendv( bool blocking, std::vector<IoVec>& ioVec, 
    MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
    MP::Communicator group, CommReq* commReq, FunctorBase_0<bool>* functor )
{
    m_dbg.verbose(CALL_INFO,1,1,"dest=%#x tag=%#x length=%lu \n",
                                        dest, tag, calcLength(ioVec) );
    m_sendState->enter( blocking, ioVec, dtype, dest, tag, group,
                                        commReq, functor );
}

void XXX::recvv( bool blocking, std::vector<IoVec>& ioVec, 
    MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
    MP::Communicator group, CommReq* commReq, FunctorBase_0<bool>* functor )
{
    m_dbg.verbose(CALL_INFO,1,1,"src=%#x tag=%#x length=%lu\n",
                                        src, tag, calcLength(ioVec) );
    m_recvState->enter( blocking, ioVec, dtype, src, tag, group,commReq, functor );
}

void XXX::waitAny( std::vector<CommReq*>& reqs,
                                FunctorBase_1<CommReq*,bool>* functor )
{
    m_waitAnyState->enter( reqs, functor );
}

void XXX::waitAll( std::vector<CommReq*>& reqs,
                                FunctorBase_1<CommReq*,bool>* functor )
{
    m_waitAllState->enter( reqs, functor );
}

void XXX::send(MP::Addr buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, FunctorBase_0<bool>* func )
{
    m_dbg.verbose(CALL_INFO,1,1,"count=%d dest=%d tag=%#x\n",count,dest,tag);

    m_processQueuesState->enterSend( new _CommReq( _CommReq::Send, buf, count,
            info()->sizeofDataType( dtype), dest, tag, group ), func );
}

void XXX::isend(MP::Addr buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req,
		FunctorBase_0<bool>* func )
{
    *req = new _CommReq( _CommReq::Isend, buf, count,
            info()->sizeofDataType(dtype) , dest, tag, group );
    m_dbg.verbose(CALL_INFO,1,1,"%p\n",*req);
    m_processQueuesState->enterSend( static_cast<_CommReq*>(*req), func );
}

void XXX::recv(MP::Addr buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageResponse* resp,
		FunctorBase_0<bool>* func )
{
    m_dbg.verbose(CALL_INFO,1,1,"count=%d src=%d tag=%#x\n",count,src,tag);
    m_processQueuesState->enterRecv( new _CommReq( _CommReq::Recv, buf, count,
            info()->sizeofDataType(dtype), src, tag, group, resp ), func);
}

void XXX::irecv(MP::Addr buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req,
        FunctorBase_0<bool>* func )
{
    m_dbg.verbose(CALL_INFO,1,1,"count=%d src=%d tag=%#x\n",count,src,tag);
    *req = new _CommReq( _CommReq::Irecv, buf, count,
            info()->sizeofDataType(dtype), src, tag, group );
    m_processQueuesState->enterRecv( static_cast<_CommReq*>(*req), func );
}

void XXX::wait( MP::MessageRequest req, MP::MessageResponse* resp,
		FunctorBase_0<bool>* func )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");

    m_processQueuesState->enterWait( new WaitReq( req, resp ), func );
}

void XXX::waitAny( int count, MP::MessageRequest req[], int *index,
        MP::MessageResponse* resp, FunctorBase_0<bool>* func  )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    m_processQueuesState->enterWait( new WaitReq( count, req, index, resp ), func );
}

void XXX::waitAll( int count, MP::MessageRequest req[],
	MP::MessageResponse* resp[], FunctorBase_0<bool>* func )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    m_processQueuesState->enterWait( new WaitReq( count, req, resp ), func );
}

void XXX::schedFunctor( FunctorBase_0<bool>* functor, uint64_t delay )
{
    m_delayLink->send( delay, new DelayEvent(functor) );
}

void XXX::schedCallback( Callback callback, uint64_t delay )
{
    m_delayLink->send( delay, new DelayEvent(callback) );
}

void XXX::delayHandler( SST::Event* e )
{
    DelayEvent* event = static_cast<DelayEvent*>(e);
    
    m_dbg.verbose(CALL_INFO,2,1,"\n");

    if ( event->functor0 ) {
        if ( (*event->functor0)( ) ) {
            delete event->functor0;
        }
    } else if ( event->functor1 ) {
        if ( (*event->functor1)( event->req ) ) {
            delete event->functor1;
        }
    } else if ( event->callback ) {
        event->callback();
    }
    delete e;
}

bool XXX::notifyGetDone( void* key )
{
    m_dbg.verbose(CALL_INFO,1,1,"key=%p\n",key);

    if ( key ) {
        Callback* callback = static_cast<Callback*>(key);
        (*callback)();
        delete callback;
    }

    return true;
}

bool XXX::notifySendPioDone( void* key )
{
    m_dbg.verbose(CALL_INFO,2,1,"key=%p\n",key);

    if ( key ) {
        Callback* callback = static_cast<Callback*>(key);
        (*callback)();
        delete callback;
    }

    return true;
}

bool XXX::notifyRecvDmaDone( int nid, int tag, size_t len, void* key )
{
    m_dbg.verbose(CALL_INFO,1,1,"src=%#x tag=%#x len=%lu key=%p\n",
                                                    nid,tag,len,key);
    if ( key ) {
        Callback2* callback = static_cast<Callback2*>(key);
        (*callback)(nid,tag,len);
        delete callback;
    }

    return true;
}

bool XXX::notifyNeedRecv(int nid, int tag, size_t len )
{

    m_dbg.verbose(CALL_INFO,1,1,"src=%#x tag=%#x len=%lu\n",nid,tag,len);
    m_processQueuesState->needRecv( nid, tag, len );
    
    return true;
}

void XXX::passCtrlToFunction( uint64_t delay, FunctorBase_1<CommReq*, bool>* functor, CommReq* req )
{
    m_dbg.verbose(CALL_INFO,1,1,"back to Function delay=%" PRIu64 " functor=%p\n",
                                delay, functor);

    if ( functor ) {
        m_delayLink->send( delay, new DelayEvent( functor, req ) );
    } else {
        m_retLink->send( delay, NULL );
    }
}

void XXX::passCtrlToFunction( uint64_t delay, FunctorBase_0<bool>* functor )
{
    m_dbg.verbose(CALL_INFO,1,1,"back to Function delay=%" PRIu64 " functor=%p\n",
                                delay, functor);

    if ( functor ) {
        m_delayLink->send( delay, new DelayEvent( functor ) );
    } else {
        m_retLink->send( delay, NULL );
    }
}

