// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
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
#include "api.h"

namespace SST {
namespace Firefly {

class InitStartEvent : public Event {
  public:
    InitStartEvent()
    { }
};

class FiniStartEvent : public Event {
  public:
    FiniStartEvent()
    { }
};

class RankStartEvent : public Event {
  public:
    RankStartEvent( Hermes::Communicator _group, int* _rank ) :
        group( _group ),
        rank( _rank )
    { }

    Hermes::Communicator group;
    int* rank;
};

class SizeStartEvent : public Event {
  public:
    SizeStartEvent( Hermes::Communicator _group, int* _size ) :
        group( _group ),
        size( _size )
    { }

    Hermes::Communicator group;
    int* size;
};

class BarrierStartEvent : public Event {
  public:
    BarrierStartEvent( Hermes::Communicator _group ) :
        group( _group )
    { }

    Hermes::Communicator group;
};

class RecvStartEvent : public Event {

  public:
    RecvStartEvent(
            Hermes::Addr _buf, uint32_t _count,
            Hermes::PayloadDataType _dtype, Hermes::RankID _src,
            uint32_t _tag, Hermes::Communicator _group, 
            Hermes::MessageRequest* _req, Hermes::MessageResponse* _resp ) :
        buf( _buf ),
        count( _count ),
        dtype( _dtype ),      
        src( _src ),
        tag( _tag ),
        group( _group ),
        resp( _resp ),
        req( _req )
    {}
    Hermes::Addr                buf;
    uint32_t                    count;
    Hermes::PayloadDataType     dtype;
    Hermes::RankID              src;
    uint32_t                    tag;
    Hermes::Communicator        group;
    Hermes::MessageResponse*    resp;
    Hermes::MessageRequest*     req;
};


class SendStartEvent : public Event {

  public:
    SendStartEvent(
                Hermes::Addr _buf, uint32_t _count,
                Hermes::PayloadDataType _dtype, Hermes::RankID _dest,
                uint32_t _tag, Hermes::Communicator _group, 
                Hermes::MessageRequest* _req ) :
        buf( _buf ),
        count( _count ),
        dtype( _dtype ),      
        dest( _dest ),
        tag( _tag ),
        group( _group ),
        req( _req )
    {}
    Hermes::Addr                buf;
    uint32_t                    count;
    Hermes::PayloadDataType     dtype;
    Hermes::RankID              dest;
    uint32_t                    tag;
    Hermes::Communicator        group;
    Hermes::MessageRequest*     req;
};

class CollectiveStartEvent : public Event {

  public:
    CollectiveStartEvent(
                Hermes::Addr _mydata, Hermes::Addr _result, uint32_t _count,
                Hermes::PayloadDataType _dtype, Hermes::ReductionOperation _op,
                Hermes::RankID _root, Hermes::Communicator _group,
                bool _all ) :
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

class GatherBaseStartEvent : public Event {

  public:
    GatherBaseStartEvent(
                Hermes::Addr _sendbuf, uint32_t _sendcnt,
                Hermes::PayloadDataType _sendtype,
                Hermes::Addr _recvbuf,
                Hermes::PayloadDataType _recvtype,
                Hermes::RankID _root, Hermes::Communicator _group ) :
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
    GatherStartEvent(
            Hermes::Addr sendbuf, uint32_t sendcnt,
            Hermes::PayloadDataType sendtype,
            Hermes::Addr recvbuf, Hermes::Addr _recvcnt, Hermes::Addr _displs,
            Hermes::PayloadDataType recvtype,
            Hermes::RankID root, Hermes::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, root, group ),
            recvcntPtr( _recvcnt),
            displsPtr( _displs ) 
    { }

    GatherStartEvent(
            Hermes::Addr sendbuf, uint32_t sendcnt,
            Hermes::PayloadDataType sendtype,
            Hermes::Addr recvbuf, uint32_t _recvcnt,
            Hermes::PayloadDataType recvtype,
            Hermes::RankID root, Hermes::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, root, group ),
            recvcntPtr( 0 ),
            displsPtr( 0 ),
            recvcnt( _recvcnt) 
    { }

    GatherStartEvent(
            Hermes::Addr sendbuf, uint32_t sendcnt,
            Hermes::PayloadDataType sendtype,
            Hermes::Addr recvbuf, uint32_t _recvcnt,
            Hermes::PayloadDataType recvtype,
            Hermes::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, 0, group ),
            recvcntPtr( 0 ),
            displsPtr( 0 ),
            recvcnt( _recvcnt) 
    { }

    GatherStartEvent(
            Hermes::Addr sendbuf, uint32_t sendcnt,
            Hermes::PayloadDataType sendtype,
            Hermes::Addr recvbuf, Hermes::Addr _recvcnt, Hermes::Addr _displs,
            Hermes::PayloadDataType recvtype, Hermes::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, 0, group ),
            recvcntPtr( _recvcnt),
            displsPtr( _displs )
    { }

    Hermes::Addr recvcntPtr;
    Hermes::Addr displsPtr;
    uint32_t recvcnt;
};

class AlltoallStartEvent: public Event {
  public:
    AlltoallStartEvent(
            Hermes::Addr _sendbuf, uint32_t _sendcnt, 
            Hermes::PayloadDataType _sendtype, 
            Hermes::Addr _recvbuf, uint32_t _recvcnt, 
            Hermes::PayloadDataType _recvtype, 
            Hermes::Communicator _group  ) :
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

    AlltoallStartEvent(
            Hermes::Addr _sendbuf, Hermes::Addr _sendcnts,  
            Hermes::Addr _senddispls,
            Hermes::PayloadDataType _sendtype, 
            Hermes::Addr _recvbuf, Hermes::Addr _recvcnts, 
            Hermes::Addr _recvdispls,
            Hermes::PayloadDataType _recvtype, 
            Hermes::Communicator _group ) :
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

class WaitStartEvent : public Event {
  public:
    WaitStartEvent(
        Hermes::MessageRequest _req, Hermes::MessageResponse* _resp ) :
        req(_req),
        resp(_resp)
    { }

    Hermes::MessageRequest req;
    Hermes::MessageResponse* resp;
};

class WaitAnyStartEvent : public Event {
  public:
    WaitAnyStartEvent( int _count, Hermes::MessageRequest _req[], 
                            int* _index, Hermes::MessageResponse* _resp ) :
        count( _count ),
        req( _req ),
        index( _index ),
        resp( _resp )
    { }

    int count;
    Hermes::MessageRequest* req;
    int* index;
    Hermes::MessageResponse* resp;
};

class WaitAllStartEvent : public Event {
  public:
    WaitAllStartEvent( int _count, Hermes::MessageRequest _req[], 
                            Hermes::MessageResponse* _resp[] ) :
        count( _count ),
        req( _req ),
        resp( _resp )
    { }

    int count;
    Hermes::MessageRequest*  req;
    Hermes::MessageResponse** resp;
};


class CommSplitStartEvent : public Event {
  public:
    CommSplitStartEvent( Hermes::Communicator _oldComm, int _color, int _key,
                            Hermes::Communicator* _newComm ) :
        oldComm(_oldComm),
        color( _color ),
        key( _key ),
        newComm( _newComm )
    { }

    Hermes::Communicator oldComm;
    int color;
    int key;
    Hermes::Communicator* newComm;
};


}
}
#endif
