// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_FIREFLY_HADESMP_H
#define COMPONENTS_FIREFLY_HADESMP_H

#include <sst/core/params.h>

#include "sst/elements/hermes/msgapi.h"
#include "hades.h"
#include "functionSM.h"

using namespace Hermes;

namespace SST {
namespace Firefly {

class HadesMP : public MP::Interface 
{
  public:
    HadesMP(Component*, Params&);
    ~HadesMP();

    virtual std::string getName() { return "HadesMP"; } 

	virtual void setup();
	virtual void finish();
	virtual void setOS( OS* os ) { 
		m_os = static_cast<Hades*>(os); 
		dbg().verbose(CALL_INFO,2,0,"\n");
	}

	int sizeofDataType( MP::PayloadDataType type ) {
		return m_os->sizeofDataType(type);
	}

    virtual void init(MP::Functor*);
    virtual void fini(MP::Functor*);
    virtual void rank(MP::Communicator group, MP::RankID* rank,
                                                    MP::Functor*);
    virtual void size(MP::Communicator group, int* size, MP::Functor* );

    virtual void send(MP::Addr buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag, 
        MP::Communicator group, MP::Functor*);

    virtual void isend(MP::Addr payload, uint32_t count, 
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req,
        MP::Functor*);

    virtual void recv(MP::Addr target, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID source, uint32_t tag,
        MP::Communicator group, MP::MessageResponse* resp,
        MP::Functor*);

    virtual void irecv(MP::Addr target, uint32_t count, 
        MP::PayloadDataType dtype, MP::RankID source, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req,
        MP::Functor*);

    virtual void allreduce(MP::Addr mydata, void* result, uint32_t count,
        MP::PayloadDataType dtype, MP::ReductionOperation op,
        MP::Communicator group, MP::Functor*);

    virtual void reduce(MP::Addr mydata, MP::Addr result,
        uint32_t count, MP::PayloadDataType dtype, 
        MP::ReductionOperation op, MP::RankID root,
        MP::Communicator group, MP::Functor*);

    virtual void bcast(MP::Addr mydata,
        uint32_t count, MP::PayloadDataType dtype, MP::RankID root,
        MP::Communicator group, MP::Functor*);

    virtual void allgather( MP::Addr sendbuf, uint32_t sendcnt, 
        MP::PayloadDataType sendtype,
        MP::Addr recvbuf, uint32_t recvcnt, 
        MP::PayloadDataType recvtype,
        MP::Communicator group, MP::Functor*);

    virtual void allgatherv( MP::Addr sendbuf, uint32_t sendcnt,
        MP::PayloadDataType sendtype,
        MP::Addr recvbuf, MP::Addr recvcnt, MP::Addr displs,
        MP::PayloadDataType recvtype,
        MP::Communicator group, MP::Functor*);

    virtual void gather( MP::Addr sendbuf, uint32_t sendcnt, 
        MP::PayloadDataType sendtype,
        MP::Addr recvbuf, uint32_t recvcnt, 
        MP::PayloadDataType recvtype,
        MP::RankID root, MP::Communicator group, MP::Functor*);

    virtual void gatherv( MP::Addr sendbuf, uint32_t sendcnt,
        MP::PayloadDataType sendtype,
        MP::Addr recvbuf, MP::Addr recvcnt, MP::Addr displs,
        MP::PayloadDataType recvtype,
        MP::RankID root, MP::Communicator group, MP::Functor*);

    virtual void barrier(MP::Communicator group, MP::Functor*);

    virtual void alltoall(
        MP::Addr sendbuf, uint32_t sendcnt, 
                        MP::PayloadDataType sendtype,
        MP::Addr recvbuf, uint32_t 
                        recvcnt, MP::PayloadDataType recvtype,
        MP::Communicator group, MP::Functor*);

    virtual void alltoallv(
        MP::Addr sendbuf, MP::Addr sendcnts, 
            MP::Addr senddispls, MP::PayloadDataType sendtype,
        MP::Addr recvbuf, MP::Addr recvcnts, 
            MP::Addr recvdispls, MP::PayloadDataType recvtype,
        MP::Communicator group, MP::Functor*);

    virtual void probe(MP::RankID source, uint32_t tag,
        MP::Communicator group, MP::MessageResponse* resp,
        MP::Functor*);

    // Added (but unused) to avoid compile warning on overloaded virtual function
    virtual void probe( int source, uint32_t tag, 
        MP::Communicator group, MP::MessageResponse* resp, MP::Functor* ) {} 

    virtual void wait(MP::MessageRequest req,
        MP::MessageResponse* resp, MP::Functor*);

    virtual void waitany( int count, MP::MessageRequest req[], int *index,
                 MP::MessageResponse* resp, MP::Functor* );
 
    virtual void waitall( int count, MP::MessageRequest req[],
                 MP::MessageResponse* resp[], MP::Functor* );

    virtual void test(MP::MessageRequest req, int& flag, 
        MP::MessageResponse* resp, MP::Functor*);

    // Added (but unused) to avoid compile warning on overloaded virtual function
    virtual void test(MP::MessageRequest* req, int& flag, MP::MessageResponse* resp,
        MP::Functor* ) {};

    virtual void comm_split( MP::Communicator, int color, int key,
        MP::Communicator*, MP::Functor* );

    virtual void comm_create( MP::Communicator, size_t nRanks, int* ranks,
        MP::Communicator*, MP::Functor* );

    virtual void comm_destroy( MP::Communicator, MP::Functor* );

  private:
	Output& dbg() { return m_os->m_dbg; }
	FunctionSM& functionSM() { return *m_functionSM; }	
	Hades*	    m_os;
	ProtocolAPI* m_proto;
	FunctionSM*  m_functionSM;
};

} // namesapce Firefly 
} // namespace SST

#endif
