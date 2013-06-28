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


#ifndef COMPONENTS_FIREFLY_TESTHERMES_H
#define COMPONENTS_FIREFLY_TESTHERMES_H

#include "sst/elements/hermes/msgapi.h"
#include "ioapi.h"
#include "msgHdr.h"
#include "group.h"

namespace SST {
namespace Firefly {

class FunctionCtx;
class BaseEntry;
class SendEntry;
class RecvEntry;
class NodeInfo;

class Hades : public Hermes::MessageInterface
{
    static const uint32_t MaxNumPostedRecvs = 32;
    static const uint32_t MaxNumSends = 32;
    typedef StaticArg_Functor<Hades, IO::Entry*, IO::Entry*>   IO_Functor;
    typedef Arg_Functor<Hades, IO::NodeId>                     IO_Functor2;
    enum { RunFunctionPre, RunRecv, RunSend, RunFunctionPost } m_state;

    class IOEntry : public IO::Entry {
      public:
        IOEntry() : callback(NULL)  { }
        ~IOEntry() { if ( callback ) delete callback; }

        BaseEntry*  entry;
        IO_Functor* callback; 
    };

    class IncomingEntry : public IO::Entry {
      public:
        IncomingEntry() : callback(NULL)  { }
        ~IncomingEntry() { if ( callback ) delete callback; }

        uint32_t                    srcNodeId;
        Hdr                         hdr;
        std::vector<IO::IoVec>      vec; 
        std::vector<unsigned char>  buffer;
        RecvEntry*                  recvEntry;
        IO_Functor*                 callback; 
    };

    class RetFuncEvent : public SST::Event {
      public:
        Hermes::Functor*    m_retFunc;
        int                 m_retval;
    };

    class MatchEvent : public SST::Event {
      public:
        IncomingEntry* incomingEntry;
    };

  public:
    Hades(Params&);
    virtual void _componentInit(unsigned int phase );
    virtual void _componentSetup();
    virtual void init(Hermes::Functor*);
    virtual void fini(Hermes::Functor*);
    virtual void rank(Hermes::Communicator group, int* rank, Hermes::Functor*);
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

    virtual void barrier(Hermes::Communicator group, Hermes::Functor*);

    virtual int probe(Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
        Hermes::Functor*);

    virtual int wait(Hermes::MessageRequest* req,
        Hermes::MessageResponse* resp, Hermes::Functor*);

    virtual int test(Hermes::MessageRequest* req, int& flag, 
        Hermes::MessageResponse* resp, Hermes::Functor*);

    void sendReturn(int delay, Hermes::Functor* retFunc, int retval ) {
        m_retFuncEvent.m_retFunc = retFunc;
        m_retFuncEvent.m_retval = retval;
        m_funcReturnDelayLink->send(delay, &m_retFuncEvent);
    }

    bool canPostSend();
    void postSendEntry(Hermes::Addr target, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageRequest* req,
        Hermes::Functor* retFunc);

    bool canPostRecv();
    void postRecvEntry(Hermes::Addr target, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
        Hermes::MessageRequest* req, Hermes::Functor* retFunc);

    std::map<Hermes::Communicator, Group*> m_groupMap;

  private:
    void run();
    void setCtx( FunctionCtx* ctx ) { 
        m_ctx = ctx;
        m_state = RunFunctionPre;
        run();
    }

    bool runRecv( );
    bool runSend( );

    IO::Entry* ioRecvBodyDone(IO::Entry*);
    IO::Entry* sendIODone(IO::Entry*);
    IO::Entry* ioRecvHdrDone(IO::Entry*);

    IO::NodeId rankToNodeId(Hermes::Communicator group, Hermes::RankID rank) {
        return m_groupMap[group]->getNodeId(rank);
    }

    void handle_event(SST::Event*);
    void handleMatchDelay(SST::Event*);
    void dataReady(IO::NodeId);

    void readHdr(IO::NodeId);
    RecvEntry* findMatch(IncomingEntry*);
    IncomingEntry* searchUnexpected(RecvEntry*); 

    Group* initAdjacentMap( int numRanks, int numCores, std::ifstream& );
    Group* initRoundRobinMap( int numRanks, int numCores, std::ifstream& );

    FunctionCtx*        m_ctx;
    SST::Link*          m_funcReturnDelayLink;  
    SST::Link*          m_matchDelayLink;  
    RetFuncEvent        m_retFuncEvent; 
    std::vector<int>    m_funcLat;
    IO::Interface*      m_io;
    NodeInfo*           m_nodeInfo;

    std::deque<IncomingEntry*>  m_unexpectedQ;
    std::deque<RecvEntry*>      m_postedQ;
    std::deque<SendEntry*>      m_sendQ;

    void setIOCallback();
    
    int myNodeId() { 
        if ( m_io ) {
            return m_io->getNodeId();
        } else {
            return -1;
        }
    }

    Hermes::RankID myWorldRank();

    Hermes::RankID _myWorldRank() {
        if ( m_groupMap.empty() ) {
            return -1; 
        } else {
            return myWorldRank();
        }
    }
};

} // namesapce Firefly 
} // namespace SST

#endif
