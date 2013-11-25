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

    class IORequest : public IO::Entry {
      public:
        int                     protoType;
        IO::NodeId              nodeId;
    };

    class SelfEvent : public SST::Event {
      public:
        SelfEvent() : Event() {}
        IORequest* aaa;
    };

    class Out : public ProtocolAPI::OutBase {
      public:
        Out( IO::Interface* obj ) : m_obj(obj) { }

        bool sendv(int dest, std::vector<IoVec>&vec, IO::Entry::Functor* func) {
            return m_obj->sendv(dest,vec,func);
        }
        bool recvv(int src, std::vector<IoVec>& vec, IO::Entry::Functor* func) {
            return m_obj->recvv(src,vec,func);
        }
        IO::Interface* m_obj;
    };

  public:
    Hades(Component*, Params&);
    virtual void printStatus( Output& );
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

    virtual void alltoall(
        Hermes::Addr sendbuf, uint32_t sendcnt, 
                        Hermes::PayloadDataType sendtype,
        Hermes::Addr recvbuf, uint32_t 
                        recvcnt, Hermes::PayloadDataType recvtype,
        Hermes::Communicator group, Hermes::Functor*);

    virtual void alltoallv(
        Hermes::Addr sendbuf, Hermes::Addr sendcnts, 
            Hermes::Addr senddispls, Hermes::PayloadDataType sendtype,
        Hermes::Addr recvbuf, Hermes::Addr recvcnts, 
            Hermes::Addr recvdispls, Hermes::PayloadDataType recvtype,
        Hermes::Communicator group, Hermes::Functor*);

    virtual void probe(Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
        Hermes::Functor*);

    virtual void wait(Hermes::MessageRequest req,
        Hermes::MessageResponse* resp, Hermes::Functor*);

    virtual void waitany( int count, Hermes::MessageRequest req[], int *index,
                 Hermes::MessageResponse* resp, Hermes::Functor* );
 
    virtual void waitall( int count, Hermes::MessageRequest req[],
                 Hermes::MessageResponse* resp[], Hermes::Functor* );

    virtual void test(Hermes::MessageRequest req, int& flag, 
        Hermes::MessageResponse* resp, Hermes::Functor*);

    Hermes::RankID myWorldRank();

  private:

    enum { WaitFunc, WaitIO } m_state;

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

    bool runSend();
    bool runRecv();

    void enterEventHandler(SST::Event*);

    IO::Entry* recvWireHdrDone(IO::Entry*);
    IO::Entry* sendWireHdrDone(IO::Entry*);

    Group* initAdjacentMap( int numRanks, int numCores, std::ifstream& );
    Group* initRoundRobinMap( int numRanks, int numCores, std::ifstream& );

    bool sendv(int dest, std::vector<IoVec>&, IO::Entry::Functor*);
    bool recvv(int src, std::vector<IoVec>&, IO::Entry::Functor*);

    SST::Link*          m_enterLink;  
    IO::Interface*      m_io;
    NodeInfo*           m_nodeInfo;
    Info                m_info;
    FunctionSM*         m_functionSM;
    Output              m_dbg;
    Out*                m_out;

    std::map<std::string,ProtocolAPI*>   m_protocolMapByName;
    std::map<int,ProtocolAPI*>           m_protocolM;

    std::map<int,ProtocolAPI*>::iterator m_sendIter;

    std::map<int,ProtocolAPI*>::iterator currentSendIterator( ) {
        return m_sendIter;
    } 

    void advanceSendIterator() {
        ++m_sendIter;
        if ( m_sendIter == m_protocolM.end() ) 
            m_sendIter = m_protocolM.begin();
    }
};

} // namesapce Firefly 
} // namespace SST

#endif
