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


#ifndef COMPONENTS_FIREFLY_HADES_H
#define COMPONENTS_FIREFLY_HADES_H

#include <sst/core/output.h>

#include "sst/elements/hermes/msgapi.h"
#include "ioapi.h"
#include "group.h"
#include "info.h"
#include "functionCtx.h"
#include "protocolAPI.h"

namespace SST {
namespace Firefly {

class FunctionSM;
class NodeInfo;
class XXX;

class Hades : public Hermes::MessageInterface
{
    typedef StaticArg_Functor<Hades, IO::Entry*, IO::Entry*>   IO_Functor;
    typedef Arg_Functor<Hades, IO::NodeId>                     IO_Functor2;

    class AAA : public IO::Entry {
    
      public:
        enum {  SendWireHdrDone,
                SendIODone,
                RecvWireHdrDone,
                RecvIODone}     ioType;
        int                     protoType;
        ProtocolAPI::Request*   request;
        IO_Functor*             functor;
        IO::NodeId              srcNodeId;
    };

    class SelfEvent : public SST::Event {
      public:
        SelfEvent() : Event() {}
        AAA* aaa;
    };

  public:
    Hades(Params&);
    virtual void _componentInit(unsigned int phase );
    virtual void _componentSetup();
    virtual void init(Hermes::Functor*);
    virtual void fini(Hermes::Functor*);
    virtual void rank(Hermes::Communicator group, Hermes::RankID* rank,
                                                    Hermes::Functor*);
    virtual void size(Hermes::Communicator group, int* size, Hermes::Functor* );

    virtual void send(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID dest, uint32_t tag, 
        Hermes::Communicator group, Hermes::Functor*);

    virtual void isend(Hermes::Addr payload, uint32_t count, 
        Hermes::PayloadDataType dtype, Hermes::RankID dest, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageRequest* req,
        Hermes::Functor*);

    virtual void recv(Hermes::Addr target, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
        Hermes::Functor*);

    virtual void irecv(Hermes::Addr target, uint32_t count, 
        Hermes::PayloadDataType dtype, Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageRequest* req,
        Hermes::Functor*);

    virtual void allreduce(Hermes::Addr mydata, void* result, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::ReductionOperation op,
        Hermes::Communicator group, Hermes::Functor*);

    virtual void reduce(Hermes::Addr mydata, Hermes::Addr result,
        uint32_t count, Hermes::PayloadDataType dtype, 
        Hermes::ReductionOperation op, Hermes::RankID root,
        Hermes::Communicator group, Hermes::Functor*);

    virtual void allgather( Hermes::Addr sendbuf, uint32_t sendcnt, 
        Hermes::PayloadDataType sendtype,
        Hermes::Addr recvbuf, uint32_t recvcnt, 
        Hermes::PayloadDataType recvtype,
        Hermes::Communicator group, Hermes::Functor*);

    virtual void allgatherv( Hermes::Addr sendbuf, uint32_t sendcnt,
        Hermes::PayloadDataType sendtype,
        Hermes::Addr recvbuf, Hermes::Addr recvcnt, Hermes::Addr displs,
        Hermes::PayloadDataType recvtype,
        Hermes::Communicator group, Hermes::Functor*);

    virtual void gather( Hermes::Addr sendbuf, uint32_t sendcnt, 
        Hermes::PayloadDataType sendtype,
        Hermes::Addr recvbuf, uint32_t recvcnt, 
        Hermes::PayloadDataType recvtype,
        Hermes::RankID root, Hermes::Communicator group, Hermes::Functor*);

    virtual void gatherv( Hermes::Addr sendbuf, uint32_t sendcnt,
        Hermes::PayloadDataType sendtype,
        Hermes::Addr recvbuf, Hermes::Addr recvcnt, Hermes::Addr displs,
        Hermes::PayloadDataType recvtype,
        Hermes::RankID root, Hermes::Communicator group, Hermes::Functor*);

    virtual void barrier(Hermes::Communicator group, Hermes::Functor*);

    virtual void probe(Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
        Hermes::Functor*);

    virtual void wait(Hermes::MessageRequest* req,
        Hermes::MessageResponse* resp, Hermes::Functor*);

    virtual void test(Hermes::MessageRequest* req, int& flag, 
        Hermes::MessageResponse* resp, Hermes::Functor*);

    Hermes::RankID myWorldRank();

  private:

    int myNodeId() { 
        if ( m_io ) {
            return m_io->getNodeId();
        } else {
            return -1;
        }
    }

    Hermes::RankID _myWorldRank() {
        return m_info.worldRank();
    }

    int sizeofDataType( Hermes::PayloadDataType type ) { 
        return m_info.sizeofDataType(type); 
    }

    int m_pendingSends;
    bool runSend();
    void runRecv();
    bool pendingSend() { return m_pendingSends > 0; }
    bool functionIsBlocked();

    IO::Entry* recvWireHdrDone(IO::Entry*);
    IO::Entry* sendWireHdrDone(IO::Entry*);

    IO::Entry* ioDone(IO::Entry*);

    IO::Entry* sendIODone(IO::Entry*);
    IO::Entry* recvIODone(IO::Entry*);
    IO::Entry* delayDone(AAA*);

    void enterEventHandler(SST::Event*);
    void leaveEventHandler(SST::Event*);

    void completedIO();
    void completedDelay();

    Group* initAdjacentMap( int numRanks, int numCores, std::ifstream& );
    Group* initRoundRobinMap( int numRanks, int numCores, std::ifstream& );

    SST::Component*     m_owner;
    SST::Link*          m_enterLink;  
    IO::Interface*      m_io;
    NodeInfo*           m_nodeInfo;
    Info                m_info;
    FunctionSM*         m_functionSM;
    Output              m_dbg;

    AAA*                m_completedIO;
    AAA*                m_delay;

    std::map<int,ProtocolAPI*>           m_protocolM;
    std::map<int,ProtocolAPI*>::iterator m_sendIter;
};

} // namesapce Firefly 
} // namespace SST

#endif
