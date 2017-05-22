// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
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

#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sstream>

#include "virtNic.h"
#include "ctrlMsgXXX.h"
#include "ctrlMsgProcessQueuesState.h"
#include "info.h"
#include "loopBack.h"

using namespace SST::Firefly::CtrlMsg;
using namespace SST::Firefly;
using namespace SST;

XXX::XXX( Component* parent, Params& params ) :
    SubComponent( parent ),
    m_retLink( NULL ),
    m_memHeapLink( NULL ),
    m_info( NULL ),
    m_rxPostMod( NULL ),
    m_processQueuesState( NULL )
{
    m_dbg_level = params.find<uint32_t>("verboseLevel",0);
    m_dbg_mask = params.find<int32_t>("verboseMask",-1);

    m_statPstdRcv = registerStatistic<uint64_t>("posted_receive_list");
    m_statRcvdMsg = registerStatistic<uint64_t>("received_msg_list");

    m_dbg.init("@t:CtrlMsg::@p():@l ", 
        m_dbg_level, 
        m_dbg_mask,
        Output::STDOUT );

    std::stringstream ss;
    ss << this;

    m_delayLink = configureSelfLink( 
        "CtrlMsgSelfLink." + ss.str(), "1 ns",
        new Event::Handler<XXX>(this,&XXX::delayHandler));

    m_matchDelay_ns = params.find<int>( "matchDelay_ns", 1 );

    std::string tmpName = params.find<std::string>("txMemcpyMod");
    Params tmpParams = params.find_prefix_params("txMemcpyModParams.");
    m_txMemcpyMod = dynamic_cast<LatencyMod*>( 
            loadModule( tmpName, tmpParams ) );  
    assert( m_txMemcpyMod );
    
    tmpName = params.find<std::string>("rxMemcpyMod");
    tmpParams = params.find_prefix_params("rxMemcpyModParams.");
    m_rxMemcpyMod = dynamic_cast<LatencyMod*>( 
            loadModule( tmpName, tmpParams ) );  
    assert( m_rxMemcpyMod );

    tmpName = params.find<std::string>("txSetupMod");
    tmpParams = params.find_prefix_params("txSetupModParams.");
    m_txSetupMod = dynamic_cast<LatencyMod*>( 
            loadModule( tmpName, tmpParams ) );  
    assert( m_txSetupMod );
    
    tmpName = params.find<std::string>("rxSetupMod");
    tmpParams = params.find_prefix_params("rxSetupModParams.");
    m_rxSetupMod = dynamic_cast<LatencyMod*>( 
            loadModule( tmpName, tmpParams ) );  
    assert( m_rxSetupMod );

    tmpName = params.find<std::string>("rxPostMod");
    if ( ! tmpName.empty() ) {
        tmpParams = params.find_prefix_params("rxPostModParams.");
        m_rxPostMod = dynamic_cast<LatencyMod*>( 
            loadModule( tmpName, tmpParams ) );  
    }

    m_regRegionBaseDelay_ns = params.find<int>( "regRegionBaseDelay_ns", 0 );
    m_regRegionPerPageDelay_ns = params.find<int>( "regRegionPerPageDelay_ns", 0 );
    m_regRegionXoverLength = params.find<int>( "regRegionXoverLength", 4096 );
    
    m_shortMsgLength = params.find<size_t>( "shortMsgLength", 4096 );

    tmpName = params.find<std::string>("txFiniMod","firefly.LatencyMod");
    tmpParams = params.find_prefix_params("txFiniModParams.");
    m_txFiniMod = dynamic_cast<LatencyMod*>( 
            loadModule( tmpName, tmpParams ) );  
    assert( m_txFiniMod );
    
    tmpName = params.find<std::string>("rxFiniMod","firefly.LatencyMod");
    tmpParams = params.find_prefix_params("rxFiniModParams.");
    m_rxFiniMod = dynamic_cast<LatencyMod*>( 
            loadModule( tmpName, tmpParams ) );  
    assert( m_rxFiniMod );
    
    m_sendAckDelay = params.find<int>( "sendAckDelay_ns", 0 );

    m_sendStateDelay = params.find<uint64_t>( "sendStateDelay_ps", 0 );
    m_recvStateDelay = params.find<uint64_t>( "recvStateDelay_ps", 0 );
    m_waitallStateDelay = params.find<uint64_t>( "waitallStateDelay_ps", 0 );
    m_waitanyStateDelay = params.find<uint64_t>( "waitanyStateDelay_ps", 0 );

    m_loopLink = configureLink(
			params.find<std::string>("loopBackPortName", "loop"), "1 ns",
            new Event::Handler<XXX>(this,&XXX::loopHandler) );
    assert(m_loopLink);
}

void XXX::finish() { 
    m_processQueuesState->finish(); 
}

XXX::~XXX()
{
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

void XXX::init( Info* info, VirtNic* nic, Thornhill::MemoryHeapLink* mem  )
{
    m_info = info;
    m_nic = nic;
    m_memHeapLink = mem;
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
    
    NotSerializable(Foo)
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

void XXX::memEventHandler( Event* ev )
{
    MemRespEvent* event = static_cast<MemRespEvent*>(ev);
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    event->callback();
    delete event;
}

// **********************************************************************


void XXX::init() {
   	m_processQueuesState->enterInit( (m_memHeapLink) );
}
void XXX::makeProgress() {
   	m_processQueuesState->enterMakeProgress( );
}

void XXX::sendv( std::vector<IoVec>& ioVec, 
    MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
    MP::Communicator group, CommReq* commReq )
{
    m_dbg.verbose(CALL_INFO,1,1,"dest=%#x tag=%#x length=%lu \n",
                                        dest, tag, calcLength(ioVec) );

    _CommReq* req;
    if ( commReq ) {
        req = new _CommReq( _CommReq::Isend, ioVec,
            info()->sizeofDataType(dtype) , dest, tag, group );
        commReq->req = req;
    } else {
        req = new _CommReq( _CommReq::Send, ioVec,
            info()->sizeofDataType( dtype), dest, tag, group );
    }

    m_processQueuesState->enterSend( req, sendStateDelay() );
}

void XXX::recvv( std::vector<IoVec>& ioVec, 
    MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
    MP::Communicator group, CommReq* commReq )
{
    m_dbg.verbose(CALL_INFO,1,1,"src=%#x tag=%#x length=%lu\n",
                                        src, tag, calcLength(ioVec) );

    _CommReq* req;
    if ( commReq ) {
        req = new _CommReq( _CommReq::Irecv, ioVec,
            info()->sizeofDataType(dtype), src, tag, group );
        commReq->req = req;
    } else {
        req = new _CommReq( _CommReq::Recv, ioVec,
            info()->sizeofDataType(dtype), src, tag, group );
    }

    m_processQueuesState->enterRecv( req, recvStateDelay() );
}

void XXX::waitAll( std::vector<CommReq*>& reqs )
{
    std::vector<_CommReq*> tmp(reqs.size());
    for ( unsigned i = 0; i < reqs.size(); i++ ) {
        tmp[i] = reqs[i]->req; 
    }
    m_processQueuesState->enterWait( new WaitReq( tmp ), waitallStateDelay() );
}

// **********************************************************************
void XXX::send(const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group )
{
    m_dbg.verbose(CALL_INFO,1,1,"count=%d dest=%d tag=%#x\n",count,dest,tag);

    m_processQueuesState->enterSend( new _CommReq( _CommReq::Send, buf, count,
            info()->sizeofDataType( dtype), dest, tag, group ) );
}

void XXX::isend(const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req )
{
    *req = new _CommReq( _CommReq::Isend, buf, count,
            info()->sizeofDataType(dtype) , dest, tag, group );
    m_dbg.verbose(CALL_INFO,1,1,"%p\n",*req);
    m_processQueuesState->enterSend( static_cast<_CommReq*>(*req) );
}

void XXX::recv(const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageResponse* resp )
{
    m_dbg.verbose(CALL_INFO,1,1,"count=%d src=%d tag=%#x\n",count,src,tag);
    m_processQueuesState->enterRecv( new _CommReq( _CommReq::Recv, buf, count,
            info()->sizeofDataType(dtype), src, tag, group, resp ) );
}

void XXX::irecv(const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req )
{
    m_dbg.verbose(CALL_INFO,1,1,"count=%d src=%d tag=%#x\n",count,src,tag);
    *req = new _CommReq( _CommReq::Irecv, buf, count,
            info()->sizeofDataType(dtype), src, tag, group );
    m_processQueuesState->enterRecv( static_cast<_CommReq*>(*req) );
}

// **********************************************************************

void XXX::wait( MP::MessageRequest req, MP::MessageResponse* resp )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");

    m_processQueuesState->enterWait( new WaitReq( req, resp ) );
}

void XXX::waitAny( int count, MP::MessageRequest req[], int *index,
        MP::MessageResponse* resp  )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    m_processQueuesState->enterWait( new WaitReq( count, req, index, resp ) );
}

void XXX::waitAll( int count, MP::MessageRequest req[],
	MP::MessageResponse* resp[] )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    m_processQueuesState->enterWait( new WaitReq( count, req, resp ) );
}

// **********************************************************************

void XXX::schedCallback( Callback callback, uint64_t delay )
{
    m_dbg.verbose(CALL_INFO,1,1,"delay=%" PRIu64 "\n",delay);
    m_delayLink->send( delay, new DelayEvent(callback) );
}

void XXX::delayHandler( SST::Event* e )
{
    DelayEvent* event = static_cast<DelayEvent*>(e);
    
    m_dbg.verbose(CALL_INFO,2,1,"execute callback\n");

    event->callback();
    delete e;
}

void XXX::memcpy( Callback callback, MemAddr to, MemAddr from, size_t length )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    if ( 0 ) {
        //m_memLink->send( 0, new MemCpyReqEvent( callback, 0, to, from, length ) );
    } else {
        uint64_t delay; 
        if ( from ) {
            delay = txMemcpyDelay( length );
        } else if ( to ) {
            delay = rxMemcpyDelay( length );
        } else {
            assert(0);
        }
        m_delayLink->send( delay, new DelayEvent(callback) );
    }
}

void XXX::memread( Callback callback, MemAddr addr, size_t length )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    if ( 0 ) {
        //m_memLink->send( 0, new MemReadReqEvent( callback, 0, addr, length ) );
    } else {
        m_delayLink->send( txMemcpyDelay( length ), new DelayEvent(callback) );
    }
}

void XXX::memwrite( Callback callback, MemAddr addr, size_t length )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    if ( 0 ) {
        //m_memLink->send( 0, new MemWriteReqEvent( callback, 0, addr, length ) );
    } else {
        m_delayLink->send( txMemcpyDelay( length ), new DelayEvent(callback) );
    }
}

void XXX::mempin( Callback callback, MemAddr addr, size_t length )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    m_delayLink->send( regRegionDelay( length ), new DelayEvent(callback) );
}

void XXX::memunpin( Callback callback, MemAddr addr, size_t length )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    m_delayLink->send( regRegionDelay( length ), new DelayEvent(callback) );
}

void XXX::memwalk( Callback callback, int count )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    m_delayLink->send( matchDelay( count ), new DelayEvent(callback) );
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

void XXX::passCtrlToFunction( uint64_t delay )
{
    m_dbg.verbose(CALL_INFO,1,1,"back to Function delay=%" PRIu64 "\n",
                                delay );

    m_retLink->send( delay, NULL ); 
}
