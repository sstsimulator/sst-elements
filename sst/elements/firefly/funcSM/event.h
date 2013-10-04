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

#ifndef COMPONENTS_FIREFLY_FUNCSM_EVENT_H
#define COMPONENTS_FIREFLY_FUNCSM_EVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include "sst/elements/hermes/msgapi.h"
#include "entry.h"
#include "api.h"

namespace SST {
namespace Firefly {

class InitStartEvent : public SMStartEvent {
  public:
    InitStartEvent( int type, Hermes::Functor* retFunc ) :
        SMStartEvent( type, retFunc )
    { }
};

class FiniStartEvent : public SMStartEvent {
  public:
    FiniStartEvent( int type, Hermes::Functor* retFunc ) :
        SMStartEvent( type, retFunc )
    { }
};

class RankStartEvent : public SMStartEvent {
  public:
    RankStartEvent( int type, Hermes::Functor* retFunc,
        Hermes::Communicator _group, int* _rank ) :
        SMStartEvent( type, retFunc ),
        group( _group ),
        rank( _rank )
    { }

    Hermes::Communicator group;
    int* rank;
};

class SizeStartEvent : public SMStartEvent {
  public:
    SizeStartEvent( int type, Hermes::Functor* retFunc,
        Hermes::Communicator _group, int* _size ) :
        SMStartEvent( type, retFunc ),
        group( _group ),
        size( _size )
    { }

    Hermes::Communicator group;
    int* size;
};

class BarrierStartEvent : public SMStartEvent {
  public:
    BarrierStartEvent( int type, Hermes::Functor* retFunc,
        Hermes::Communicator _group ) :
        SMStartEvent( type, retFunc ),
        group( _group )
    { }

    Hermes::Communicator group;
};

class RecvStartEvent : public SMStartEvent {

  public:
    RecvStartEvent( int type, Hermes::Functor* retFunc,
            Hermes::Addr buf, uint32_t count,
            Hermes::PayloadDataType dtype, Hermes::RankID src,
            uint32_t tag, Hermes::Communicator group, 
            Hermes::MessageRequest* req, Hermes::MessageResponse* resp ) :
        SMStartEvent( type, retFunc ),
        entry( buf, count, dtype, src, tag, group, resp, req )  
    {}

    RecvEntry   entry;
};


class SendStartEvent : public SMStartEvent {

  public:
    SendStartEvent( int type, Hermes::Functor* retFunc,
                Hermes::Addr buf, uint32_t count,
                Hermes::PayloadDataType dtype, Hermes::RankID dest,
                uint32_t tag, Hermes::Communicator group, 
                Hermes::MessageRequest* req ) :
        SMStartEvent( type, retFunc ),
        entry( buf, count, dtype, dest, tag, group, req )  
    {}

    SendEntry   entry;
};

class CollectiveStartEvent : public SMStartEvent {

  public:
    CollectiveStartEvent(int type, Hermes::Functor* retFunc,
                Hermes::Addr _mydata, Hermes::Addr _result, uint32_t _count,
                Hermes::PayloadDataType _dtype, Hermes::ReductionOperation _op,
                Hermes::RankID _root, Hermes::Communicator _group,
                bool _all ) :
        SMStartEvent( type, retFunc ),
        mydata(_mydata),
        result(_result),
        count(_count),
        dtype(_dtype),
        op(_op),
        root(_root),
        group(_group),
        all( _all ) 
    {}
    
    Hermes::Addr mydata;
    Hermes::Addr result;
    uint32_t count;
    Hermes::PayloadDataType dtype;
    Hermes::ReductionOperation op;
    Hermes::RankID  root;
    Hermes::Communicator group;
    bool  all;
};

class GatherBaseStartEvent : public SMStartEvent {

  public:
    GatherBaseStartEvent(int type, Hermes::Functor* retFunc,
        Hermes::Addr _sendbuf, uint32_t _sendcnt,
        Hermes::PayloadDataType _sendtype,
        Hermes::Addr _recvbuf,
        Hermes::PayloadDataType _recvtype,
        Hermes::RankID _root, Hermes::Communicator _group ) :

        SMStartEvent( type, retFunc ),
        sendbuf(_sendbuf),
        recvbuf(_recvbuf),
        sendcnt(_sendcnt),
        sendtype(_sendtype),
        recvtype(_recvtype),
        root(_root),
        group(_group)
    {}
    
    Hermes::Addr sendbuf;
    Hermes::Addr recvbuf;
    uint32_t sendcnt;
    Hermes::PayloadDataType sendtype;
    Hermes::PayloadDataType recvtype;
    Hermes::RankID  root;
    Hermes::Communicator group;
};

class GatherStartEvent : public GatherBaseStartEvent {
  public:
    GatherStartEvent(int type, Hermes::Functor* retFunc,
            Hermes::Addr sendbuf, uint32_t sendcnt,
            Hermes::PayloadDataType sendtype,
            Hermes::Addr recvbuf, Hermes::Addr _recvcnt, Hermes::Addr _displs,
            Hermes::PayloadDataType recvtype,
            Hermes::RankID root, Hermes::Communicator group ) :
        GatherBaseStartEvent( type, retFunc, sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, root, group ),
            recvcntPtr( _recvcnt),
            displsPtr( _displs ) 
    { }

    GatherStartEvent(int type, Hermes::Functor* retFunc,
            Hermes::Addr sendbuf, uint32_t sendcnt,
            Hermes::PayloadDataType sendtype,
            Hermes::Addr recvbuf, uint32_t _recvcnt,
            Hermes::PayloadDataType recvtype,
            Hermes::RankID root, Hermes::Communicator group ) :
        GatherBaseStartEvent( type, retFunc, sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, root, group ),
            recvcntPtr( 0 ),
            displsPtr( 0 ),
            recvcnt( _recvcnt) 
    { }

    GatherStartEvent(int type, Hermes::Functor* retFunc,
            Hermes::Addr sendbuf, uint32_t sendcnt,
            Hermes::PayloadDataType sendtype,
            Hermes::Addr recvbuf, uint32_t _recvcnt,
            Hermes::PayloadDataType recvtype,
            Hermes::Communicator group ) :
        GatherBaseStartEvent( type, retFunc, sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, 0, group ),
            recvcntPtr( 0 ),
            displsPtr( 0 ),
            recvcnt( _recvcnt) 
    { }

    GatherStartEvent(int type, Hermes::Functor* retFunc,
            Hermes::Addr sendbuf, uint32_t sendcnt,
            Hermes::PayloadDataType sendtype,
            Hermes::Addr recvbuf, Hermes::Addr _recvcnt, Hermes::Addr _displs,
            Hermes::PayloadDataType recvtype, Hermes::Communicator group ) :
        GatherBaseStartEvent( type, retFunc, sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, 0, group ),
            recvcntPtr( _recvcnt),
            displsPtr( _displs )
    { }

    Hermes::Addr recvcntPtr;
    Hermes::Addr displsPtr;
    uint32_t recvcnt;
};

class AlltoallStartEvent: public SMStartEvent {
  public:
    AlltoallStartEvent( int type, Hermes::Functor* retFunc,
            Hermes::Addr _sendbuf, uint32_t _sendcnt, 
            Hermes::PayloadDataType _sendtype, 
            Hermes::Addr _recvbuf, uint32_t _recvcnt, 
            Hermes::PayloadDataType _recvtype, 
            Hermes::Communicator _group  ) :
        SMStartEvent( type, retFunc ),
        sendbuf( _sendbuf ),
        sendcnt( _sendcnt ),
        sendcnts( NULL ),
        sendtype( _sendtype ),
        recvbuf( _recvbuf ),
        recvcnt( _recvcnt ),
        recvcnts( NULL ),
        recvtype( _recvtype ),
        group( _group )
    { } 

    AlltoallStartEvent( int type, Hermes::Functor* retFunc,
            Hermes::Addr _sendbuf, Hermes::Addr _sendcnts,  
            Hermes::Addr _senddispls,
            Hermes::PayloadDataType _sendtype, 
            Hermes::Addr _recvbuf, Hermes::Addr _recvcnts, 
            Hermes::Addr _recvdispls,
            Hermes::PayloadDataType _recvtype, 
            Hermes::Communicator _group ) :
        SMStartEvent( type, retFunc ),
        sendbuf( _sendbuf ),
        sendcnts( _sendcnts ),
        senddispls( _senddispls ),
        sendtype( _sendtype ),
        recvbuf( _recvbuf ),
        recvcnts( _recvcnts ),
        recvdispls( _recvdispls ),
        recvtype( _recvtype ),
        group( _group )
    { } 

    Hermes::Addr            sendbuf;
    uint32_t                sendcnt;
    Hermes::Addr            sendcnts;
    Hermes::Addr            senddispls;
    Hermes::PayloadDataType sendtype;
    Hermes::Addr            recvbuf;
    uint32_t                recvcnt;
    Hermes::Addr            recvcnts;
    Hermes::Addr            recvdispls;
    Hermes::PayloadDataType recvtype;
    Hermes::Communicator    group;
};

class WaitStartEvent : public SMStartEvent {
  public:
    WaitStartEvent( int type, Hermes::Functor* retFunc,
        Hermes::MessageRequest* _req, Hermes::MessageResponse* _resp ) :
        SMStartEvent( type, retFunc ),
        req(_req),
        resp(_resp)
    { }

    Hermes::MessageRequest* req;
    Hermes::MessageResponse* resp;
};

}
}
#endif
