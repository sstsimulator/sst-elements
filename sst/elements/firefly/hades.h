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

class Hades : public MP::Interface
{
  public:
    Hades(Component*, Params&);
    ~Hades();
    virtual void printStatus( Output& );
    virtual void _componentInit(unsigned int phase );
    virtual void _componentSetup();
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

    virtual void wait(MP::MessageRequest req,
        MP::MessageResponse* resp, MP::Functor*);

    virtual void waitany( int count, MP::MessageRequest req[], int *index,
                 MP::MessageResponse* resp, MP::Functor* );
 
    virtual void waitall( int count, MP::MessageRequest req[],
                 MP::MessageResponse* resp[], MP::Functor* );

    virtual void test(MP::MessageRequest req, int& flag, 
        MP::MessageResponse* resp, MP::Functor*);

    virtual void comm_split( MP::Communicator, int color, int key,
        MP::Communicator*, MP::Functor* );


    MP::RankID myWorldRank();
    int myWorldSize();

  private:

    int sizeofDataType( MP::PayloadDataType type ) { 
        return m_info.sizeofDataType(type); 
    }

    void initAdjacentMap( std::istream&, Group*, int numCores );

    SST::Link*          m_enterLink;  
    VirtNic*            m_virtNic;
    Info                m_info;
    FunctionSM*         m_functionSM;
    Output              m_dbg;

    std::map<std::string,ProtocolAPI*>   m_protocolMapByName;
    std::map<int,ProtocolAPI*>           m_protocolM;
    std::string                          m_nidListString;
};

} // namesapce Firefly 
} // namespace SST

#endif
