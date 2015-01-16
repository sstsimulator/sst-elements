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

using namespace Hermes;

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
    RankStartEvent( MP::Communicator _group, int* _rank ) :
        group( _group ),
        rank( _rank )
    { }

    MP::Communicator group;
    int* rank;
};

class SizeStartEvent : public Event {
  public:
    SizeStartEvent( MP::Communicator _group, int* _size ) :
        group( _group ),
        size( _size )
    { }

    MP::Communicator group;
    int* size;
};

class BarrierStartEvent : public Event {
  public:
    BarrierStartEvent( MP::Communicator _group ) :
        group( _group )
    { }

    MP::Communicator group;
};

class RecvStartEvent : public Event {

  public:
    RecvStartEvent(
            MP::Addr _buf, uint32_t _count,
            MP::PayloadDataType _dtype, MP::RankID _src,
            uint32_t _tag, MP::Communicator _group, 
            MP::MessageRequest* _req, MP::MessageResponse* _resp ) :
        buf( _buf ),
        count( _count ),
        dtype( _dtype ),      
        src( _src ),
        tag( _tag ),
        group( _group ),
        resp( _resp ),
        req( _req )
    {}
    MP::Addr                buf;
    uint32_t                    count;
    MP::PayloadDataType     dtype;
    MP::RankID              src;
    uint32_t                    tag;
    MP::Communicator        group;
    MP::MessageResponse*    resp;
    MP::MessageRequest*     req;
};


class SendStartEvent : public Event {

  public:
    SendStartEvent(
                MP::Addr _buf, uint32_t _count,
                MP::PayloadDataType _dtype, MP::RankID _dest,
                uint32_t _tag, MP::Communicator _group, 
                MP::MessageRequest* _req ) :
        buf( _buf ),
        count( _count ),
        dtype( _dtype ),      
        dest( _dest ),
        tag( _tag ),
        group( _group ),
        req( _req )
    {}
    MP::Addr                buf;
    uint32_t                    count;
    MP::PayloadDataType     dtype;
    MP::RankID              dest;
    uint32_t                    tag;
    MP::Communicator        group;
    MP::MessageRequest*     req;
};

class CollectiveStartEvent : public Event {

  public:
    CollectiveStartEvent(
                MP::Addr _mydata, MP::Addr _result, uint32_t _count,
                MP::PayloadDataType _dtype, MP::ReductionOperation _op,
                MP::RankID _root, MP::Communicator _group,
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
    
    MP::Addr mydata;
    MP::Addr result;
    uint32_t count;
    MP::PayloadDataType dtype;
    MP::ReductionOperation op;
    MP::RankID  root;
    MP::Communicator group;
    bool  all;
};

class GatherBaseStartEvent : public Event {

  public:
    GatherBaseStartEvent(
                MP::Addr _sendbuf, uint32_t _sendcnt,
                MP::PayloadDataType _sendtype,
                MP::Addr _recvbuf,
                MP::PayloadDataType _recvtype,
                MP::RankID _root, MP::Communicator _group ) :
        sendbuf(_sendbuf),
        recvbuf(_recvbuf),
        sendcnt(_sendcnt),
        sendtype(_sendtype),
        recvtype(_recvtype),
        root(_root),
        group(_group)
    {}
    
    MP::Addr sendbuf;
    MP::Addr recvbuf;
    uint32_t sendcnt;
    MP::PayloadDataType sendtype;
    MP::PayloadDataType recvtype;
    MP::RankID  root;
    MP::Communicator group;
};

class GatherStartEvent : public GatherBaseStartEvent {
  public:
    GatherStartEvent(
            MP::Addr sendbuf, uint32_t sendcnt,
            MP::PayloadDataType sendtype,
            MP::Addr recvbuf, MP::Addr _recvcnt, MP::Addr _displs,
            MP::PayloadDataType recvtype,
            MP::RankID root, MP::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, root, group ),
            recvcntPtr( _recvcnt),
            displsPtr( _displs ) 
    { }

    GatherStartEvent(
            MP::Addr sendbuf, uint32_t sendcnt,
            MP::PayloadDataType sendtype,
            MP::Addr recvbuf, uint32_t _recvcnt,
            MP::PayloadDataType recvtype,
            MP::RankID root, MP::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, root, group ),
            recvcntPtr( 0 ),
            displsPtr( 0 ),
            recvcnt( _recvcnt) 
    { }

    GatherStartEvent(
            MP::Addr sendbuf, uint32_t sendcnt,
            MP::PayloadDataType sendtype,
            MP::Addr recvbuf, uint32_t _recvcnt,
            MP::PayloadDataType recvtype,
            MP::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, 0, group ),
            recvcntPtr( 0 ),
            displsPtr( 0 ),
            recvcnt( _recvcnt) 
    { }

    GatherStartEvent(
            MP::Addr sendbuf, uint32_t sendcnt,
            MP::PayloadDataType sendtype,
            MP::Addr recvbuf, MP::Addr _recvcnt, MP::Addr _displs,
            MP::PayloadDataType recvtype, MP::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, 0, group ),
            recvcntPtr( _recvcnt),
            displsPtr( _displs )
    { }

    MP::Addr recvcntPtr;
    MP::Addr displsPtr;
    uint32_t recvcnt;
};

class AlltoallStartEvent: public Event {
  public:
    AlltoallStartEvent(
            MP::Addr _sendbuf, uint32_t _sendcnt, 
            MP::PayloadDataType _sendtype, 
            MP::Addr _recvbuf, uint32_t _recvcnt, 
            MP::PayloadDataType _recvtype, 
            MP::Communicator _group  ) :
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
            MP::Addr _sendbuf, MP::Addr _sendcnts,  
            MP::Addr _senddispls,
            MP::PayloadDataType _sendtype, 
            MP::Addr _recvbuf, MP::Addr _recvcnts, 
            MP::Addr _recvdispls,
            MP::PayloadDataType _recvtype, 
            MP::Communicator _group ) :
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

    MP::Addr            sendbuf;
    uint32_t                sendcnt;
    MP::Addr            sendcnts;
    MP::Addr            senddispls;
    MP::PayloadDataType sendtype;
    MP::Addr            recvbuf;
    uint32_t                recvcnt;
    MP::Addr            recvcnts;
    MP::Addr            recvdispls;
    MP::PayloadDataType recvtype;
    MP::Communicator    group;
};

class WaitStartEvent : public Event {
  public:
    WaitStartEvent(
        MP::MessageRequest _req, MP::MessageResponse* _resp ) :
        req(_req),
        resp(_resp)
    { }

    MP::MessageRequest req;
    MP::MessageResponse* resp;
};

class WaitAnyStartEvent : public Event {
  public:
    WaitAnyStartEvent( int _count, MP::MessageRequest _req[], 
                            int* _index, MP::MessageResponse* _resp ) :
        count( _count ),
        req( _req ),
        index( _index ),
        resp( _resp )
    { }

    int count;
    MP::MessageRequest* req;
    int* index;
    MP::MessageResponse* resp;
};

class WaitAllStartEvent : public Event {
  public:
    WaitAllStartEvent( int _count, MP::MessageRequest _req[], 
                            MP::MessageResponse* _resp[] ) :
        count( _count ),
        req( _req ),
        resp( _resp )
    { }

    int count;
    MP::MessageRequest*  req;
    MP::MessageResponse** resp;
};


class CommSplitStartEvent : public Event {
  public:
    CommSplitStartEvent( MP::Communicator _oldComm, int _color, int _key,
                            MP::Communicator* _newComm ) :
        oldComm(_oldComm),
        color( _color ),
        key( _key ),
        newComm( _newComm )
    { }

    MP::Communicator oldComm;
    int color;
    int key;
    MP::Communicator* newComm;
};

class CommCreateStartEvent : public Event {
  public:
    CommCreateStartEvent( MP::Communicator _oldComm, size_t _nRanks, 
                int* _ranks, MP::Communicator* _newComm ) :
        oldComm(_oldComm),
        nRanks( _nRanks ),
        ranks( _ranks ),
        newComm( _newComm )
    { }

    MP::Communicator oldComm;
    size_t nRanks;
    int*   ranks;
    MP::Communicator* newComm;
};

class CommDestroyStartEvent : public Event {
  public:
    CommDestroyStartEvent( MP::Communicator _comm ) :
        comm(_comm)
    { }

    MP::Communicator comm;
};


}
}
#endif
