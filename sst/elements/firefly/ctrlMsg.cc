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

#include "ctrlMsg.h"
#include "ctrlMsgXXX.h"
#include "nic.h"
#include "info.h"

using namespace SST::Firefly;
using namespace SST;
using namespace SST::Firefly::CtrlMsg;

API::API( Component* owner, Params& params ) 
{
    m_xxx = new XXX( owner, params );
    assert( m_xxx );
}

API::~API()
{
    delete m_xxx;
}

void API::init( Info* info, VirtNic* nic )
{
    m_xxx->init( info, nic );
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

void API::send( void* buf, size_t len, nid_t dest, uint64_t tag,
                                            FunctorBase_0<bool>* functor ) 
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;

    m_xxx->sendv( true, ioVec, MP::CHAR, dest,
                tag, MP::GroupWorld, NULL, functor );
}

void API::send( void* buf, size_t len, MP::RankID dest, MP::Communicator grp,
                            uint64_t tag, FunctorBase_0<bool>* functor ) 
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;

    m_xxx->sendv( true, ioVec, MP::CHAR, dest,
                tag, grp, NULL, functor );
}

void API::isend( void* buf, size_t len, nid_t dest, uint64_t tag, CommReq* req,
                                            FunctorBase_0<bool>* functor ) 
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;

    m_xxx->sendv( false, ioVec, MP::CHAR, dest,
                tag, MP::GroupWorld, req, functor );
}

void API::sendv(std::vector<IoVec>& ioVec, nid_t dest, uint64_t tag,
                                                FunctorBase_0<bool>* functor )
{
    m_xxx->sendv( true, ioVec, MP::CHAR, dest,
                tag, MP::GroupWorld, NULL, functor );
}


void API::isendv(std::vector<IoVec>& ioVec, nid_t dest, uint64_t tag,
                            CommReq* req, FunctorBase_0<bool>* functor )
{
    m_xxx->sendv( false, ioVec, MP::CHAR, dest,
                tag, MP::GroupWorld, req, functor );
}

void API::recv( void* buf, size_t len, nid_t src, uint64_t tag,
                                            FunctorBase_0<bool>* functor )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;
    m_xxx->recvv( true, ioVec, MP::CHAR, src,
                tag, MP::GroupWorld, NULL, functor );
}

void API::irecv( void* buf, size_t len, nid_t src, uint64_t tag,
                                    CommReq* req, FunctorBase_0<bool>* functor )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;
    m_xxx->recvv( false, ioVec, MP::CHAR, src,
                tag, MP::GroupWorld, req, functor );
}

void API::irecv( void* buf, size_t len, MP::RankID src, MP::Communicator grp,
                        uint64_t tag, CommReq* req, FunctorBase_0<bool>* functor )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;
    m_xxx->recvv( false, ioVec, MP::CHAR, src,
                tag, grp, req, functor );
}

void API::irecvv(std::vector<IoVec>& ioVec, nid_t src, uint64_t tag,
                            CommReq* req, FunctorBase_0<bool>* functor )
{
    m_xxx->recvv( false, ioVec, MP::CHAR, src,
                tag, MP::GroupWorld, req, functor );
}

void API::wait( CommReq* req, FunctorBase_1<CommReq*,bool>* functor )
{
    std::vector<CommReq*> tmp; 
    tmp.push_back( req );
    m_xxx->waitAny( tmp, functor );
}

void API::waitAny( std::vector<CommReq*>& reqs, FunctorBase_1<CommReq*,bool>* functor )
{
    m_xxx->waitAny( reqs, functor );
}

void API::waitAll( std::vector<CommReq*>& reqs, FunctorBase_1<CommReq*,bool>* functor )
{
    m_xxx->waitAll( reqs, functor );
}

void API::send(MP::Addr buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, FunctorBase_0<bool>* func )
{
	m_xxx->send( buf, count, dtype, dest, tag, group, func );
}

void API::isend(MP::Addr buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req,
		FunctorBase_0<bool>* func )
{
	m_xxx->isend( buf, count, dtype, dest, tag, group, req, func );
}

void API::recv(MP::Addr buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageResponse* resp,
		FunctorBase_0<bool>* func )
{
	m_xxx->recv( buf, count, dtype, src, tag, group, resp, func ); 
}

void API::irecv(MP::Addr buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req,
        FunctorBase_0<bool>* func )
{
	m_xxx->irecv( buf, count, dtype, src, tag, group, req, func ); 
}

void API::wait( MP::MessageRequest req, MP::MessageResponse* resp,
		FunctorBase_0<bool>* func )
{
	m_xxx->wait( req, resp, func );
}
void API::waitAny( int count, MP::MessageRequest req[], int *index,
       	MP::MessageResponse* resp, FunctorBase_0<bool>* func )
{
	m_xxx->waitAny( count, req, index, resp, func );
}

void API::waitAll( int count, MP::MessageRequest req[],
        MP::MessageResponse* resp[], FunctorBase_0<bool>* func )
{
	m_xxx->waitAll( count, req, resp, func );
}

size_t API::shortMsgLength()
{
    return m_xxx->shortMsgLength();
}
