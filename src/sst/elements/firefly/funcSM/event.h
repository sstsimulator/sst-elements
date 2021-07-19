// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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

    NotSerializable(InitStartEvent)
};

class FiniStartEvent : public Event {
  public:
    FiniStartEvent()
    { }

    NotSerializable(FiniStartEvent)
};

class RankStartEvent : public Event {
  public:
    RankStartEvent( MP::Communicator _group, int* _rank ) :
        group( _group ),
        rank( _rank )
    { }

    MP::Communicator group;
    int* rank;

    NotSerializable(RankStartEvent)
};

class SizeStartEvent : public Event {
  public:
    SizeStartEvent( MP::Communicator _group, int* _size ) :
        group( _group ),
        size( _size )
    { }

    MP::Communicator group;
    int* size;

    NotSerializable(SizeStartEvent)
};

class MakeProgressStartEvent : public Event {
  public:
    MakeProgressStartEvent()
    { }

    NotSerializable(InitStartEvent)
};

class BarrierStartEvent : public Event {
  public:
    BarrierStartEvent( MP::Communicator _group ) :
        group( _group )
    { }

    MP::Communicator group;

    NotSerializable(BarrierStartEvent)
};

class RecvStartEvent : public Event {

  public:
    RecvStartEvent(
            const Hermes::MemAddr& _buf, uint32_t _count,
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
    Hermes::MemAddr            buf;
    uint32_t                    count;
    MP::PayloadDataType     dtype;
    MP::RankID              src;
    uint32_t                    tag;
    MP::Communicator        group;
    MP::MessageResponse*    resp;
    MP::MessageRequest*     req;

    NotSerializable(RecvStartEvent)
};


class SendStartEvent : public Event {

  public:
    SendStartEvent(
                const Hermes::MemAddr& _buf, uint32_t _count,
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
    Hermes::MemAddr         buf;
    uint32_t                count;
    MP::PayloadDataType     dtype;
    MP::RankID              dest;
    uint32_t                tag;
    MP::Communicator        group;
    MP::MessageRequest*     req;

    NotSerializable(SendStartEvent)
};

class CollectiveStartEvent : public Event {

  public:
    enum Type { Allreduce, Reduce, Bcast };
    CollectiveStartEvent(
                const Hermes::MemAddr& _mydata, const Hermes::MemAddr& _result, uint32_t _count,
                MP::PayloadDataType _dtype, MP::ReductionOperation _op,
                MP::RankID _root, MP::Communicator _group,
                Type _type ) :
        mydata(_mydata),
        result(_result),
        count(_count),
        dtype(_dtype),
        op(_op),
        root(_root),
        group(_group),
        type( _type )
    {}

    const char* typeName() {
        switch( type ) {
          case Allreduce:
            return "Allreduce";
          case Reduce:
            return "Reduce";
          case Bcast:
            return "Bcast";
        }
        return NULL;
    }

    Hermes::MemAddr mydata;
    Hermes::MemAddr result;
    uint32_t count;
    MP::PayloadDataType dtype;
    MP::ReductionOperation op;
    MP::RankID  root;
    MP::Communicator group;
    Type  type;

    NotSerializable(CollectiveStartEvent)
};

class ScattervStartEvent : public Event {

  public:
    ScattervStartEvent(
               const Hermes::MemAddr& sendBuf, int sendCnt, MP::PayloadDataType sendType,
               const Hermes::MemAddr& recvBuf, int recvCnt, MP::PayloadDataType recvType,
               MP::RankID root, MP::Communicator group ) :
        sendBuf(sendBuf),
        sendCnt(sendCnt),
        sendType(sendType),
        sendCntPtr(0),
        sendDisplsPtr(0),
        recvBuf(recvBuf),
        recvCnt(recvCnt),
        recvType(recvType),
        root(root),
        group(group)
    {}

    ScattervStartEvent(
               const Hermes::MemAddr& sendBuf, int* sendCntPtr, int* displs, MP::PayloadDataType sendType,
               const Hermes::MemAddr& recvBuf, int recvCnt, MP::PayloadDataType recvType,
               MP::RankID root, MP::Communicator group ) :
        sendBuf(sendBuf),
        sendCntPtr(sendCntPtr),
        sendDisplsPtr(displs),
        sendType(sendType),
        recvBuf(recvBuf),
        recvCnt(recvCnt),
        recvType(recvType),
        root(root),
        group(group)
    {}

    Hermes::MemAddr sendBuf;
    Hermes::MemAddr recvBuf;
    uint32_t sendCnt;
    uint32_t recvCnt;
    MP::PayloadDataType sendType;
    MP::PayloadDataType recvType;
    MP::RankID  root;
    MP::Communicator group;

    int* sendCntPtr;
    int* sendDisplsPtr;

    NotSerializable(ScattervStartEvent)
};

class GatherBaseStartEvent : public Event {

  public:
    GatherBaseStartEvent(
                const Hermes::MemAddr& _sendbuf, uint32_t _sendcnt,
                MP::PayloadDataType _sendtype,
                const Hermes::MemAddr& _recvbuf,
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

    Hermes::MemAddr sendbuf;
    Hermes::MemAddr recvbuf;
    uint32_t sendcnt;
    MP::PayloadDataType sendtype;
    MP::PayloadDataType recvtype;
    MP::RankID  root;
    MP::Communicator group;

    NotSerializable(GatherBaseStartEvent)
};

class GatherStartEvent : public GatherBaseStartEvent {
  public:
    GatherStartEvent(
            const Hermes::MemAddr& sendbuf, uint32_t sendcnt,
            MP::PayloadDataType sendtype,
            const Hermes::MemAddr& recvbuf, void* _recvcnt,  void* _displs,
            MP::PayloadDataType recvtype,
            MP::RankID root, MP::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, root, group ),
            recvcntPtr( _recvcnt),
            displsPtr( _displs )
    { }

    GatherStartEvent(
            const Hermes::MemAddr& sendbuf, uint32_t sendcnt,
            MP::PayloadDataType sendtype,
            const Hermes::MemAddr& recvbuf, uint32_t _recvcnt,
            MP::PayloadDataType recvtype,
            MP::RankID root, MP::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, root, group ),
            recvcntPtr( 0 ),
            displsPtr( 0 ),
            recvcnt( _recvcnt)
    { }

    GatherStartEvent(
            const Hermes::MemAddr& sendbuf, uint32_t sendcnt,
            MP::PayloadDataType sendtype,
            const Hermes::MemAddr& recvbuf, uint32_t _recvcnt,
            MP::PayloadDataType recvtype,
            MP::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, 0, group ),
            recvcntPtr( 0 ),
            displsPtr( 0 ),
            recvcnt( _recvcnt)
    { }

    GatherStartEvent(
            const Hermes::MemAddr& sendbuf, uint32_t sendcnt,
            MP::PayloadDataType sendtype,
            const Hermes::MemAddr& recvbuf,  void* _recvcnt, void* _displs,
            MP::PayloadDataType recvtype, MP::Communicator group ) :
        GatherBaseStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvtype, 0, group ),
            recvcntPtr( _recvcnt),
            displsPtr( _displs )
    { }

    void* recvcntPtr;
    void* displsPtr;
    uint32_t recvcnt;

    NotSerializable(GatherStartEvent)
};

class AlltoallStartEvent: public Event {
  public:
    AlltoallStartEvent(
            const Hermes::MemAddr& _sendbuf, uint32_t _sendcnt,
            MP::PayloadDataType _sendtype,
            const Hermes::MemAddr& _recvbuf, uint32_t _recvcnt,
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
            const Hermes::MemAddr& _sendbuf, void* _sendcnts,
            void* _senddispls,
            MP::PayloadDataType _sendtype,
            const Hermes::MemAddr& _recvbuf, void* _recvcnts,
            void* _recvdispls,
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

    Hermes::MemAddr            sendbuf;
    uint32_t                sendcnt;
    void*            sendcnts;
    void*            senddispls;
    MP::PayloadDataType sendtype;
    Hermes::MemAddr            recvbuf;
    uint32_t                recvcnt;
    void*            recvcnts;
    void*            recvdispls;
    MP::PayloadDataType recvtype;
    MP::Communicator    group;

    NotSerializable(AlltoallStartEvent)
};

class CancelStartEvent : public Event {
  public:
    CancelStartEvent(
        MP::MessageRequest req ) :
        req(req)
    { }

    MP::MessageRequest req;

    NotSerializable(CancelStartEvent)
};

class TestStartEvent : public Event {
  public:
    TestStartEvent(
        MP::MessageRequest _req, int* flag, MP::MessageResponse* _resp ) :
        req(_req),
		flag(flag),
        resp(_resp)
    { }

    MP::MessageRequest req;
    MP::MessageResponse* resp;
	int* flag;

    NotSerializable(TestStartEvent)
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

    NotSerializable(WaitStartEvent)
};

class TestanyStartEvent : public Event {
  public:
    TestanyStartEvent( int count, MP::MessageRequest req[],
                            int* index, int* flag, MP::MessageResponse* resp ) :
        count( count ),
        req( req ),
        index( index ),
        flag( flag ),
        resp( resp )
    { }

    int count;
    MP::MessageRequest* req;
    int* index;
	int* flag;
    MP::MessageResponse* resp;

    NotSerializable(TestanyStartEvent)
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

    NotSerializable(WaitAnyStartEvent)
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

    NotSerializable(WaitAllStartEvent)
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

    NotSerializable(CommSplitStartEvent)
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

    NotSerializable(CommCreateStartEvent)
};

class CommDestroyStartEvent : public Event {
  public:
    CommDestroyStartEvent( MP::Communicator _comm ) :
        comm(_comm)
    { }

    MP::Communicator comm;

    NotSerializable(CommDestroyStartEvent)
};


}
}
#endif
