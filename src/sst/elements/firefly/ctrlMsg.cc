// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
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

#include "ctrlMsg.h"
#include "nic.h"
#include "info.h"

#include "ctrlMsgProcessQueuesState.h"
#include "ctrlMsgMemory.h"

using namespace SST::Firefly;
using namespace SST;
using namespace SST::Firefly::CtrlMsg;

API::API( Component* owner, Params& params ) : 
    ProtocolAPI( owner ), m_memHeapLink(NULL)
{

    m_dbg_level = params.find<uint32_t>("verboseLevel",0);
    m_dbg_mask = params.find<int32_t>("verboseMask",-1);

    m_dbg.init("@t:CtrlMsg::@p():@l ",
        m_dbg_level,
        m_dbg_mask,
        Output::STDOUT );

    m_processQueuesState = dynamic_cast<ProcessQueuesState*>( owner->loadSubComponent( "firefly.ctrlMsg", owner, params) );

    m_mem = new Memory( owner, params );
    static_cast<Memory*>(m_mem)->setOutput( &m_dbg );

    m_sendStateDelay = params.find<uint64_t>( "sendStateDelay_ps", 0 );
    m_recvStateDelay = params.find<uint64_t>( "recvStateDelay_ps", 0 );
    m_waitallStateDelay = params.find<uint64_t>( "waitallStateDelay_ps", 0 );
    m_waitanyStateDelay = params.find<uint64_t>( "waitanyStateDelay_ps", 0 );
}

API::~API()
{
    if ( m_processQueuesState) delete m_processQueuesState;
    delete m_mem;
}

void API::setup() { 
    m_processQueuesState->setup();
}
void API::finish() { 
    m_processQueuesState->finish();
}

void API::setVars( Info* info, VirtNic* nic, Thornhill::MemoryHeapLink* mem, Link* retLink )
{
    m_info = info;

    nic->setNotifyOnGetDone(
        new VirtNic::Handler<API,void*>(this, &API::notifyGetDone )
    );
    nic->setNotifyOnSendPioDone(
        new VirtNic::Handler<API,void*>(this, &API::notifySendPioDone )
    );
    nic->setNotifyOnRecvDmaDone(
        new VirtNic::Handler4Args<API,int,int,size_t,void*>(
                                this, &API::notifyRecvDmaDone )
    );
    nic->setNotifyNeedRecv(
        new VirtNic::Handler2Args<API,int,size_t>(
                                this, &API::notifyNeedRecv )
    );

    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:CtrlMsg::API::@p():@l ", nic->getRealNodeId(), m_info->worldRank());
    m_dbg.setPrefix(buffer);

    m_processQueuesState->setVars( nic, m_info, m_mem, m_memHeapLink, retLink );
}

void API::makeProgress() {
	m_processQueuesState->enterMakeProgress();
}

void API::initMsgPassing() {
	m_processQueuesState->enterInit((m_memHeapLink));
}

static size_t calcLength( std::vector<IoVec>& ioVec )
{
    size_t len = 0;
    for ( size_t i = 0; i < ioVec.size(); i++ ) {
        len += ioVec[i].len;
    }
    return len;
}

void API::sendv_common( std::vector<IoVec>& ioVec,
    MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
    MP::Communicator group, CommReq* commReq )
{
    m_dbg.debug(CALL_INFO,1,1,"dest=%#x tag=%#x length=%lu \n",
                                        dest, tag, calcLength(ioVec) );

    _CommReq* req;
    if ( commReq ) {
        req = new _CommReq( _CommReq::Isend, ioVec,
            m_info->sizeofDataType(dtype) , dest, tag, group );
        commReq->req = req;
    } else {
        req = new _CommReq( _CommReq::Send, ioVec,
            m_info->sizeofDataType( dtype), dest, tag, group );
    }

    m_processQueuesState->enterSend( req, sendStateDelay() );
}

void API::send( const Hermes::MemAddr& addr, size_t len, nid_t dest, uint64_t tag )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;

    sendv_common( ioVec, MP::CHAR, dest, tag, MP::GroupWorld, NULL );
}


void API::send( const Hermes::MemAddr& addr, size_t len, MP::RankID dest, uint64_t tag, 
                        MP::Communicator grp ) 
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;

    sendv_common( ioVec, MP::CHAR, dest, tag, grp, NULL );
}

void API::isend( const Hermes::MemAddr& addr, size_t len, nid_t dest, uint64_t tag, CommReq* req)
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;

    assert(req);
    sendv_common( ioVec, MP::CHAR, dest, tag, MP::GroupWorld, req );
}

void API::isend( const Hermes::MemAddr& addr, size_t len, nid_t dest, uint64_t tag, 
				MP::Communicator group, CommReq* req)
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;

    assert(req);
    sendv_common( ioVec, MP::CHAR, dest, tag, group, req );
}

void API::sendv(std::vector<IoVec>& ioVec, nid_t dest, uint64_t tag )
{
    sendv_common( ioVec, MP::CHAR, dest, tag, MP::GroupWorld, NULL );
}

void API::recvv_common( std::vector<IoVec>& ioVec,
    MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
    MP::Communicator group, CommReq* commReq )
{
    m_dbg.debug(CALL_INFO,1,1,"src=%#x tag=%#x length=%lu\n",
                                        src, tag, calcLength(ioVec) );

    _CommReq* req;
    if ( commReq ) {
        req = new _CommReq( _CommReq::Irecv, ioVec,
            m_info->sizeofDataType(dtype), src, tag, group );
        commReq->req = req;
    } else {
        req = new _CommReq( _CommReq::Recv, ioVec,
            m_info->sizeofDataType(dtype), src, tag, group );
    }

    m_processQueuesState->enterRecv( req, recvStateDelay() );
}

void API::recv( const Hermes::MemAddr& addr, size_t len, nid_t src, uint64_t tag )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr; 
    ioVec[0].len = len;
    recvv_common( ioVec, MP::CHAR, src, tag, MP::GroupWorld, NULL );
}

void API::recv( const Hermes::MemAddr& addr, size_t len, nid_t src, uint64_t tag,
			MP::Communicator group)
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr; 
    ioVec[0].len = len;
    recvv_common( ioVec, MP::CHAR, src, tag, group, NULL );
}


void API::irecv( const Hermes::MemAddr& addr, size_t len, nid_t src, uint64_t tag, CommReq* req )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;

    assert(req);
    recvv_common( ioVec, MP::CHAR, src, tag, MP::GroupWorld, req );
}

void API::irecv( const Hermes::MemAddr& addr, size_t len, MP::RankID src, uint64_t tag, 
                                    MP::Communicator grp, CommReq* req )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;
    assert(req);
    recvv_common( ioVec, MP::CHAR, src, tag, grp, req );
}

void API::irecvv(std::vector<IoVec>& ioVec, nid_t src, uint64_t tag,
                            CommReq* req  )
{
    recvv_common( ioVec, MP::CHAR, src, tag, MP::GroupWorld, req );
}

void API::wait( CommReq* req )
{
    std::vector<CommReq*> tmp; 
    tmp.push_back( req );
    waitAll( tmp );
}

void API::waitAll( std::vector<CommReq*>& reqs )
{
    std::vector<_CommReq*> tmp(reqs.size());
    for ( unsigned i = 0; i < reqs.size(); i++ ) {
        tmp[i] = reqs[i]->req;
    }
    m_processQueuesState->enterWait( new WaitReq( tmp ), waitallStateDelay() );
}

void API::send( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group )
{
    m_dbg.debug(CALL_INFO,1,1,"count=%d dest=%d tag=%#x\n",count,dest,tag);

    m_processQueuesState->enterSend( new _CommReq( _CommReq::Send, buf, count,
            m_info->sizeofDataType( dtype), dest, tag, group ) );
}

void API::isend( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req )
{
    *req = new _CommReq( _CommReq::Isend, buf, count,
                        m_info->sizeofDataType(dtype) , dest, tag, group );
    m_dbg.debug(CALL_INFO,1,1,"%p\n",*req);
    m_processQueuesState->enterSend( static_cast<_CommReq*>(*req) );
}

void API::recv( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageResponse* resp )
{
    m_dbg.debug(CALL_INFO,1,1,"count=%d src=%d tag=%#x\n",count,src,tag);
    m_processQueuesState->enterRecv( new _CommReq( _CommReq::Recv, buf, count,
            m_info->sizeofDataType(dtype), src, tag, group, resp ) );
}

void API::irecv( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req )
{
    m_dbg.debug(CALL_INFO,1,1,"count=%d src=%d tag=%#x\n",count,src,tag);
    *req = new _CommReq( _CommReq::Irecv, buf, count,
                            m_info->sizeofDataType(dtype), src, tag, group );
    m_processQueuesState->enterRecv( static_cast<_CommReq*>(*req) );
}


void API::wait( MP::MessageRequest req, MP::MessageResponse* resp )
{
    m_dbg.debug(CALL_INFO,1,1,"\n");
    m_processQueuesState->enterWait( new WaitReq( req, resp ) );
}

void API::waitAny( int count, MP::MessageRequest req[], int *index,
       	MP::MessageResponse* resp )
{
    m_dbg.debug(CALL_INFO,1,1,"\n");
    m_processQueuesState->enterWait( new WaitReq( count, req, index, resp ) );
}

void API::waitAll( int count, MP::MessageRequest req[],
        MP::MessageResponse* resp[] )
{
    m_dbg.debug(CALL_INFO,1,1,"\n");
    m_processQueuesState->enterWait( new WaitReq( count, req, resp ) );    
}

// **********************************************************************

bool API::notifyGetDone( void* key )
{
    m_dbg.debug(CALL_INFO,1,1,"key=%p\n",key);

    if ( key ) {
        Callback* callback = static_cast<Callback*>(key);
        (*callback)();
        delete callback;
    }

    return true;
}

bool API::notifySendPioDone( void* key )
{
    m_dbg.debug(CALL_INFO,2,1,"key=%p\n",key);

    if ( key ) {
        Callback* callback = static_cast<Callback*>(key);
        (*callback)();
        delete callback;
    }

    return true;
}

bool API::notifyRecvDmaDone( int nid, int tag, size_t len, void* key )
{
    m_dbg.debug(CALL_INFO,1,1,"src=%#x tag=%#x len=%lu key=%p\n",
                                                    nid,tag,len,key);
    if ( key ) {
        Callback2* callback = static_cast<Callback2*>(key);
        (*callback)(nid,tag,len);
        delete callback;
    }

    return true;
}

bool API::notifyNeedRecv(int nid, size_t len )
{

    m_dbg.debug(CALL_INFO,1,1,"src=%#x len=%lu\n",nid,len);
    m_processQueuesState->needRecv( nid, len );

    return true;
}
