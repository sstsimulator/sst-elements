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

#include "ctrlMsg.h"
#include "ctrlMsgXXX.h"
#include "nic.h"
#include "info.h"

using namespace SST::Firefly;
using namespace SST;
using namespace SST::Firefly::CtrlMsg;

API::API( Component* owner, Params& params ) : ProtocolAPI( owner ) 
{
    m_xxx = new XXX( owner, params );
    assert( m_xxx );
}

API::~API()
{
    delete m_xxx;
}

void API::finish() { 
    m_xxx->finish();
}

void API::init( Info* info, VirtNic* nic, Thornhill::MemoryHeapLink* mem )
{
    m_xxx->init( info, nic, mem );
}

void API::setup() 
{
    m_xxx->setup();
}

Info* API::info() 
{
    return m_xxx->info();
}

void API::setRetLink( Link* link ) 
{
    m_xxx->setRetLink( link );
}

void API::init() {
	m_xxx->init();
}

void API::makeProgress() {
	m_xxx->makeProgress();
}

void API::send( const Hermes::MemAddr& addr, size_t len, nid_t dest, uint64_t tag )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;

    m_xxx->sendv( ioVec, MP::CHAR, dest, tag, MP::GroupWorld, NULL );
}


void API::send( const Hermes::MemAddr& addr, size_t len, MP::RankID dest, uint64_t tag, 
                        MP::Communicator grp ) 
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;

    m_xxx->sendv( ioVec, MP::CHAR, dest, tag, grp, NULL );
}

void API::isend( const Hermes::MemAddr& addr, size_t len, nid_t dest, uint64_t tag, CommReq* req)
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;

    assert(req);
    m_xxx->sendv( ioVec, MP::CHAR, dest, tag, MP::GroupWorld, req );
}

void API::sendv(std::vector<IoVec>& ioVec, nid_t dest, uint64_t tag )
{
    m_xxx->sendv( ioVec, MP::CHAR, dest, tag, MP::GroupWorld, NULL );
}

void API::recv( const Hermes::MemAddr& addr, size_t len, nid_t src, uint64_t tag )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr; 
    ioVec[0].len = len;
    m_xxx->recvv( ioVec, MP::CHAR, src, tag, MP::GroupWorld, NULL );
}

void API::irecv( const Hermes::MemAddr& addr, size_t len, nid_t src, uint64_t tag, CommReq* req )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;

    assert(req);
    m_xxx->recvv( ioVec, MP::CHAR, src, tag, MP::GroupWorld, req );
}

void API::irecv( const Hermes::MemAddr& addr, size_t len, MP::RankID src, uint64_t tag, 
                                    MP::Communicator grp, CommReq* req )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].addr = addr;
    ioVec[0].len = len;
    assert(req);
    m_xxx->recvv( ioVec, MP::CHAR, src, tag, grp, req );
}

void API::irecvv(std::vector<IoVec>& ioVec, nid_t src, uint64_t tag,
                            CommReq* req  )
{
    m_xxx->recvv( ioVec, MP::CHAR, src,
                tag, MP::GroupWorld, req );
}

void API::wait( CommReq* req )
{
    std::vector<CommReq*> tmp; 
    tmp.push_back( req );
    m_xxx->waitAll( tmp );
}

void API::waitAll( std::vector<CommReq*>& reqs )
{
    m_xxx->waitAll( reqs );
}

void API::send( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group )
{
	m_xxx->send( buf, count, dtype, dest, tag, group );
}

void API::isend( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req )
{
	m_xxx->isend( buf, count, dtype, dest, tag, group, req );
}

void API::recv( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageResponse* resp )
{
	m_xxx->recv( buf, count, dtype, src, tag, group, resp ); 
}

void API::irecv( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req )
{
	m_xxx->irecv( buf, count, dtype, src, tag, group, req ); 
}

void API::wait( MP::MessageRequest req, MP::MessageResponse* resp )
{
	m_xxx->wait( req, resp );
}
void API::waitAny( int count, MP::MessageRequest req[], int *index,
       	MP::MessageResponse* resp )
{
	m_xxx->waitAny( count, req, index, resp );
}

void API::waitAll( int count, MP::MessageRequest req[],
        MP::MessageResponse* resp[] )
{
	m_xxx->waitAll( count, req, resp );
}
