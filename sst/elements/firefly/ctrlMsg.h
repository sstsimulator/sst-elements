
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

#ifndef COMPONENTS_FIREFLY_CTRLMSG_H
#define COMPONENTS_FIREFLY_CTRLMSG_H

#include <sst/core/component.h>
#include "protocolAPI.h"
#include "ctrlMsgFunctors.h"
#include "ioVec.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

typedef int  nid_t;
typedef uint32_t     tag_t;

static const tag_t AllgatherTag  = 0x10000000;
static const tag_t AlltoallvTag  = 0x20000000;
static const tag_t CollectiveTag = 0x30000000;
static const tag_t GathervTag    = 0x40000000;
static const tag_t LongProtoTag  = 0x50000000;
static const tag_t TagMask       = (1<<28) - 1;

struct Status {
    nid_t   nid;
    tag_t   tag; 
    size_t  length;
};

class XXX;

class _CommReq;

struct CommReq {
    _CommReq* req;
    Status status;
    void* usrPtr;
};

static const nid_t  AnyNid = -1;
static const tag_t  AnyTag = -1; 
typedef int region_t;

typedef int RegionEvent;

class API : public ProtocolAPI {

  public:
    API( Component* owner, Params& );

    virtual void init( Info* info, VirtNic* );
    virtual void setup();
    virtual Info* info();

    virtual std::string name() { return "CtrlMsg"; }
    virtual void setRetLink( Link* link );

    void send( void* buf, size_t len, nid_t dest, tag_t tag, 
                                        FunctorBase_0<bool>* = NULL );
    void sendv( std::vector<IoVec>&, nid_t dest, tag_t tag,
                                        FunctorBase_0<bool>* = NULL );
    void isendv( std::vector<IoVec>&, nid_t dest, tag_t tag, CommReq*,
                                        FunctorBase_0<bool>* = NULL );
    void recv( void* buf, size_t len, nid_t src, tag_t tag,
                                        FunctorBase_0<bool>* = NULL );
    void recvv( std::vector<IoVec>&, nid_t src, tag_t tag,
                                        FunctorBase_0<bool>* = NULL );
    void irecv( void* buf, size_t len, nid_t src, tag_t tag, CommReq*,
                                        FunctorBase_0<bool>* = NULL );
    void irecvv( std::vector<IoVec>&, nid_t src, tag_t tag, CommReq*,
                                        FunctorBase_0<bool>* = NULL );
    void irecvv( std::vector<IoVec>&, nid_t src, tag_t tag, tag_t ignoreBits,
                                    CommReq*, FunctorBase_0<bool>* = NULL );
    void wait( CommReq*, FunctorBase_1<CommReq*,bool>* = NULL );
    void waitAny( std::vector<CommReq*>&, FunctorBase_1<CommReq*, bool>* = NULL );
    void setUsrPtr( CommReq*, void* );
    void* getUsrPtr( CommReq* );
    void getStatus( CommReq*, CtrlMsg::Status* );

    void read( nid_t, region_t, void* buf, size_t len, 
                                    FunctorBase_0<bool>* = NULL );
    size_t shortMsgLength();

    void registerRegion( region_t, nid_t, void* buf, size_t len, FunctorBase_0<bool>* = NULL );
    void unregisterRegion( region_t, FunctorBase_0<bool>* = NULL );

  private:
    XXX*    m_xxx;
};

}
}
}

#endif
