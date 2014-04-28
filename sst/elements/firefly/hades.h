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


#ifndef COMPONENTS_FIREFLY_HADES_H
#define COMPONENTS_FIREFLY_HADES_H

#include <sst/core/output.h>
#include <sst/core/params.h>

#include "sst/elements/hermes/msgapi.h"
#include "group.h"
#include "info.h"
#include "protocolAPI.h"

namespace SST {
namespace Firefly {

class FunctionSM;
class NodeInfo;
class VirtNic;

class Hades : public Hermes::MessageInterface
{
  public:
    Hades(Component*, Params&);
    ~Hades();
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
    int myWorldSize();

  private:

    int myNodeId();

    int sizeofDataType( Hermes::PayloadDataType type ) { 
        return m_info.sizeofDataType(type); 
    }

    Group* initAdjacentMap( std::istream& );

    SST::Link*          m_enterLink;  
    VirtNic*            m_virtNic;
    Info                m_info;
    FunctionSM*         m_functionSM;
    Output              m_dbg;
    SST::Params         m_params;

    std::map<std::string,ProtocolAPI*>   m_protocolMapByName;
    std::map<int,ProtocolAPI*>           m_protocolM;
};

} // namesapce Firefly 
} // namespace SST

#endif
