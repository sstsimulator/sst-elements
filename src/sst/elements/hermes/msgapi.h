// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_HERMES_MESSAGE_INTERFACE
#define _H_HERMES_MESSAGE_INTERFACE

#include <assert.h>

#include "hermes.h"

using namespace SST;

namespace SST {

namespace Hermes {

namespace MP {

typedef void*    Addr;
typedef uint32_t Communicator;
typedef uint32_t RankID;

enum PayloadDataType {
    CHAR,
	INT,
	LONG,
	DOUBLE,
	FLOAT,
	COMPLEX
};

typedef struct MessageResponse {
    uint32_t        tag;  
    RankID          src; 
    uint32_t        count;
    bool            status;
} MessageResponse;

class MessageRequestBase {
  public:
    virtual ~MessageRequestBase() {};
};

typedef MessageRequestBase* MessageRequest;


enum ReductionOperation {
    NOP,
    SUM,
    MIN,
    MAX
};

typedef Arg_FunctorBase< int, bool > Functor; 

static const uint32_t AnyTag = -1;
static const uint32_t AnySrc = -1;
static const Communicator GroupMachine = 0;
static const Communicator GroupWorld = 1;

enum Retval {
    SUCCESS,
    FAILURE
};

class Interface : public Hermes::Interface {
    public:

    Interface( Component* parent ) : Hermes::Interface( parent )  {}
    virtual ~Interface() {}

    virtual int sizeofDataType( PayloadDataType ) { assert(0); }

    virtual void init(Functor*) {}
    virtual void fini(Functor*) {}
    virtual void rank(Communicator group, RankID* rank, Functor*) {}
    virtual void size(Communicator group, int* size, Functor*) {}
    virtual void makeProgress(Functor*) {}

    virtual void send(const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, 
        RankID dest, uint32_t tag, Communicator group, 
        Functor* ) {}

    virtual void isend(const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, 
        RankID dest, uint32_t tag, Communicator group, 
        MessageRequest* req, Functor* ) {}

    virtual void recv(const Hermes::MemAddr&, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group, 
        MessageResponse* resp, Functor*) {}

    virtual void irecv(const Hermes::MemAddr&, uint32_t count, PayloadDataType dtype, 
        RankID source, uint32_t tag, Communicator group, 
        MessageRequest* req, Functor*) {}

    virtual void allreduce(const Hermes::MemAddr&, const Hermes::MemAddr&, uint32_t count, 
        PayloadDataType dtype, ReductionOperation op, 
        Communicator group, Functor*) {}

    virtual void reduce(const Hermes::MemAddr&, const Hermes::MemAddr&, uint32_t count, 
        PayloadDataType dtype, ReductionOperation op, RankID root, 
        Communicator group, Functor*) {}

    virtual void bcast(const Hermes::MemAddr&, uint32_t count, 
        PayloadDataType dtype, RankID root, 
        Communicator group, Functor*) {}

    virtual void allgather(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor*) {}

    virtual void allgatherv(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, Addr recvcnt, Addr displs, PayloadDataType recvtype,
        Communicator group, Functor*) {}

    virtual void gather(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, uint32_t recvcnt, PayloadDataType recvtype,
        RankID root, Communicator group, Functor*) {}

    virtual void gatherv(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, Addr recvcnt, Addr displs, PayloadDataType recvtype,
        RankID root, Communicator group, Functor*) {}

    virtual void alltoall(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor*) {}

    virtual void alltoallv(
        const Hermes::MemAddr& sendbuf, Addr sendcnts, Addr senddispls, PayloadDataType sendtype,
        const Hermes::MemAddr& recvbuf, Addr recvcnts, Addr recvdispls, PayloadDataType recvtype,
        Communicator group, Functor*) {}

    virtual void barrier(Communicator group, Functor*) {}

    virtual void probe( int source, uint32_t tag, 
        Communicator group, MessageResponse* resp, Functor* ) {} 

    virtual void wait( MessageRequest req, MessageResponse* resp,
        Functor* ) {};

    virtual void waitany( int count, MessageRequest req[], int *index,
                MessageResponse* resp, Functor* ) {};

    virtual void waitall( int count, MessageRequest req[],
                MessageResponse* resp[], Functor* ) {};

    virtual void test( MessageRequest* req, int& flag, MessageResponse* resp,
        Functor* ) {};

    virtual void comm_split( Communicator, int color, int key,
        Communicator*, Functor* ) {}

    virtual void comm_create( Communicator, size_t nRanks, int *ranks,
        Communicator*, Functor* ) {}

    virtual void comm_destroy( Communicator, Functor* ) {}
};

}
}
}

#endif

