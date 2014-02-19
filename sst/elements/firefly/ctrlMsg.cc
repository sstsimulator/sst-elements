// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "ctrlMsg.h"
#include "ctrlMsgXXX.h"

using namespace SST::Firefly;
using namespace SST;
using namespace SST::Firefly::CtrlMsg;

API::API( Component* owner, Params& params ) 
{
    m_xxx = new XXX( owner, params );
}

void API::init( Info* info, Nic::VirtNic* nic )
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

void API::send( void* buf, size_t len, nid_t dest, tag_t tag,
                                            FunctorBase_0<bool>* functor ) 
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;

    m_xxx->sendv( true, ioVec, dest, tag, NULL, functor );
}
void API::sendv(std::vector<IoVec>& ioVec, nid_t dest, tag_t tag,
                                                FunctorBase_0<bool>* functor )
{
    m_xxx->sendv( true, ioVec, dest, tag, NULL, functor );
}


void API::isendv(std::vector<IoVec>& ioVec, nid_t dest, tag_t tag,
                            CommReq* req, FunctorBase_0<bool>* functor )
{
    m_xxx->sendv( false, ioVec, dest, tag, req, functor );
}

void API::recv( void* buf, size_t len, nid_t src, tag_t tag,
                                            FunctorBase_0<bool>* functor )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;
    m_xxx->recvv( true, ioVec, src, tag, NULL, functor );
}

void API::irecv( void* buf, size_t len, nid_t src, tag_t tag,
                                    CommReq* req, FunctorBase_0<bool>* functor )
{
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;
    m_xxx->recvv( false, ioVec, src, tag, req, functor );
}

void API::irecvv(std::vector<IoVec>& ioVec, nid_t src, tag_t tag,
                            CommReq* req, FunctorBase_0<bool>* functor )
{
    m_xxx->recvv( false, ioVec, src, tag, req, functor );
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

void API::read( nid_t nid, region_t region, void* buf, size_t len,
                                    FunctorBase_0<bool>* functor )
{
    m_xxx->read( nid, region, buf, len, functor );
}


RegionEventQ* API::createEventQueue()
{
    return m_xxx->createEventQueue();
}

void API::registerRegion( region_t region, nid_t nid, void* buf, size_t len, RegionEventQ* q )
{
    m_xxx->registerRegion( region, nid, buf, len, q ); 
}

void API::unregisterRegion( region_t region )
{
    m_xxx->unregisterRegion( region );
}

void API::setUsrPtr( CommReq* req, void* ptr )
{
    req->usrPtr = ptr;
}

void* API::getUsrPtr( CommReq* req )
{
    return req->usrPtr;
}

void API::getStatus( CommReq* req, Status* status )
{
    *status = req->status;
}

size_t API::shortMsgLength()
{
    return m_xxx->shortMsgLength();
}
