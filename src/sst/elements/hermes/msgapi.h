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
    uint32_t		dtypeSize;
    bool            status;
} MessageResponse;

class MessageRequestBase {
  public:
    virtual ~MessageRequestBase() {};
};

typedef MessageRequestBase* MessageRequest;

enum ReductionOpType { Nop, Sum, Min, Max, Func };

typedef void (User_function)(void* a, void* b, int* len, PayloadDataType* );

struct _ReductionOperation {
	_ReductionOperation( ReductionOpType type ) : type(type) { }
	_ReductionOperation( User_function* userFunction, int commute ) :
			userFunction(userFunction), type(ReductionOpType::Func), commute(commute) { }
	ReductionOpType  type;
	User_function* userFunction;
	int commute;
};

typedef _ReductionOperation* ReductionOperation;

static _ReductionOperation* NOP = new _ReductionOperation(ReductionOpType::Nop);
static _ReductionOperation* SUM = new _ReductionOperation(ReductionOpType::Sum);
static _ReductionOperation* MIN = new _ReductionOperation(ReductionOpType::Min);
static _ReductionOperation* MAX = new _ReductionOperation(ReductionOpType::Max);

typedef Arg_FunctorBase< int, bool > Functor;

static const uint32_t AnyTag = -1;
static const uint32_t AnySrc = -1;
static const Communicator GroupWorld = 0;

enum Retval {
    SUCCESS,
    FAILURE
};

inline ReductionOperation Op_create( User_function func, int commute ) {
	return new _ReductionOperation( func, commute );
}

inline void Op_free( ReductionOperation op ) {
	delete op;
}

class Interface : public Hermes::Interface {
    public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::Hermes::MP::Interface,SST::Hermes::Interface)

    Interface( ComponentId_t id ) : Hermes::Interface( id )  {}
    virtual ~Interface() {}

    virtual int sizeofDataType( PayloadDataType ) { assert(0); }

    virtual void init(Functor*) { assert(0); }
    virtual void fini(Functor*) { assert(0); }
    virtual void rank(Communicator group, RankID* rank, Functor*) { assert(0); }
    virtual void size(Communicator group, int* size, Functor*) { assert(0); }
    virtual void makeProgress(Functor*) { assert(0); }

    virtual void send(const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype,
        RankID dest, uint32_t tag, Communicator group,
        Functor* ) { assert(0); }

    virtual void isend(const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype,
        RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* ) { assert(0); }

    virtual void recv(const Hermes::MemAddr&, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageResponse* resp, Functor*) { assert(0); }

    virtual void irecv(const Hermes::MemAddr&, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageRequest* req, Functor*) { assert(0); }

    virtual void allreduce(const Hermes::MemAddr&, const Hermes::MemAddr&, uint32_t count,
        PayloadDataType dtype, ReductionOperation op,
        Communicator group, Functor*) { assert(0); }

    virtual void reduce(const Hermes::MemAddr&, const Hermes::MemAddr&, uint32_t count,
        PayloadDataType dtype, ReductionOperation op, RankID root,
        Communicator group, Functor*) { assert(0); }

    virtual void bcast(const Hermes::MemAddr&, uint32_t count,
        PayloadDataType dtype, RankID root,
        Communicator group, Functor*) { assert(0); }

    virtual void scatter(
        const Hermes::MemAddr& sendBuf, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr& recvBuf, uint32_t recvcnt, PayloadDataType recvType,
        RankID root, Communicator group, Functor*) { assert(0); }

    virtual void scatterv(
        const Hermes::MemAddr& sendBuf, int* sendcnt, int* displs, PayloadDataType sendtype,
        const Hermes::MemAddr& recvBuf, int recvcnt, PayloadDataType recvType,
        RankID root, Communicator group, Functor*) { assert(0); }

    virtual void allgather(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor*) { assert(0); }

    virtual void allgatherv(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, Addr recvcnt, Addr displs, PayloadDataType recvtype,
        Communicator group, Functor*) { assert(0); }

    virtual void gather(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, uint32_t recvcnt, PayloadDataType recvtype,
        RankID root, Communicator group, Functor*) { assert(0); }

    virtual void gatherv(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, Addr recvcnt, Addr displs, PayloadDataType recvtype,
        RankID root, Communicator group, Functor*) { assert(0); }

    virtual void alltoall(
        const Hermes::MemAddr&, uint32_t sendcnt, PayloadDataType sendtype,
        const Hermes::MemAddr&, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor*) { assert(0); }

    virtual void alltoallv(
        const Hermes::MemAddr& sendbuf, Addr sendcnts, Addr senddispls, PayloadDataType sendtype,
        const Hermes::MemAddr& recvbuf, Addr recvcnts, Addr recvdispls, PayloadDataType recvtype,
        Communicator group, Functor*) { assert(0);}

    virtual void barrier(Communicator group, Functor*) { assert(0); }

    virtual void probe( int source, uint32_t tag,
        Communicator group, MessageResponse* resp, Functor* ) { assert(0); }

    virtual void cancel( MessageRequest req, Functor* ) { assert(0); };

    virtual void wait( MessageRequest req, MessageResponse* resp, Functor* ) { assert(0); };

    virtual void waitany( int count, MessageRequest req[], int *index, MessageResponse* resp, Functor* ) { assert(0); };

    virtual void waitall( int count, MessageRequest req[], MessageResponse* resp[], Functor* ) { assert(0); };

    virtual void test( MessageRequest req, int* flag, MessageResponse* resp, Functor* ) { assert(0); };

    virtual void testany( int count, MessageRequest req[], int* indx, int* flag, MessageResponse* resp,
        Functor* ) { assert(0); };

    virtual void comm_split( Communicator, int color, int key,
        Communicator*, Functor* ) { assert(0); }

    virtual void comm_create( Communicator, size_t nRanks, int *ranks,
        Communicator*, Functor* ) { assert(0); }

    virtual void comm_destroy( Communicator, Functor* ) { assert(0); }
};

}
}
}

#endif

