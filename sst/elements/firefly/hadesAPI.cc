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

#include <sst_config.h>
#include "sst/core/serialization.h"

#include "hades.h"
#include "functionSM.h"
#include "funcSM/event.h"

using namespace SST::Firefly;
using namespace Hermes;

void Hades::init(Functor* retFunc )
{
    m_functionSM->start( new InitStartEvent( FunctionSM::Init, retFunc) );
}

void Hades::fini(Functor* retFunc)
{
    m_functionSM->start( new FiniStartEvent( FunctionSM::Fini, retFunc) );
}

void Hades::rank(Communicator group, RankID* rank, Functor* retFunc)
{
    m_functionSM->start( new RankStartEvent(FunctionSM::Rank,
                                        retFunc, group, (int*) rank) );
}

void Hades::size(Communicator group, int* size, Functor* retFunc )
{
    m_functionSM->start( new SizeStartEvent(FunctionSM::Size, 
                                        retFunc, group, size) );
}

void Hades::send(Addr buf, uint32_t count, 
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group,
        Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p count=%d dtype=%d dest=%d tag=%d \
            group=%d\n", buf,count,dtype,dest,tag,group);
    m_functionSM->start( new SendStartEvent( FunctionSM::Send, 
         retFunc, buf, count, dtype, dest, tag, group, NULL) ); 
}

void Hades::isend(Addr buf, uint32_t count, PayloadDataType dtype,
        RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p count=%d dtype=%d dest=%d tag=%d \
            group=%d\n", buf,count,dtype,dest,tag,group);
    m_functionSM->start( new SendStartEvent( FunctionSM::Send, 
         retFunc, buf, count, dtype, dest, tag, group, req ) );
}

void Hades::recv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageResponse* resp, Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"target=%p count=%d dtype=%d source=%d tag=%d \
            group=%d\n", target,count,dtype,source,tag,group);
    m_functionSM->start( new RecvStartEvent( FunctionSM::Recv, 
            retFunc, target, count, dtype, source, tag, group, NULL, resp ) );
}

void Hades::irecv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"target=%p count=%d dtype=%d source=%d tag=%d \
            group=%d\n", target,count,dtype,source,tag,group);
    m_functionSM->start( new RecvStartEvent( FunctionSM::Recv, 
        retFunc, target, count, dtype, source, tag, group, req, NULL ) );
}

void Hades::allreduce(Addr mydata, Addr result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op,
        Communicator group, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"in=%p out=%p count=%d dtype=%d \n",
                mydata,result,count,dtype);
    m_functionSM->start( new CollectiveStartEvent(FunctionSM::Allreduce,
                         retFunc, mydata, result, count, 
                        dtype, op, 0, group, true) );
}

void Hades::reduce(Addr mydata, Addr result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op, RankID root,
        Communicator group, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"in=%p out=%p count=%d dtype=%d \n",
                mydata,result,count,dtype);
    m_functionSM->start( new CollectiveStartEvent(FunctionSM::Reduce,
                         retFunc, mydata, result, count, 
                        dtype, op, root, group, false) );
}

void Hades::allgather( Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor* retFunc)
{
    m_functionSM->start( new GatherStartEvent(FunctionSM::Allgather, retFunc,
            sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, recvtype, group ) );
}

void Hades::allgatherv(
        Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, Addr recvcnt, Addr displs, PayloadDataType recvtype,
        Communicator group, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"sendbuf=%p recvbuf=%p sendcnt=%d "
        "recvcntPtr=%p\n", sendbuf,recvbuf,sendcnt,recvcnt);
    m_functionSM->start( new GatherStartEvent(FunctionSM::Allgatherv, retFunc,
            sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, displs, recvtype, group ) );
}

void Hades::gather( Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
        RankID root, Communicator group, Functor* retFunc)
{
    m_functionSM->start( new GatherStartEvent(FunctionSM::Gather, retFunc,
            sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, recvtype, root, group ) );
}

void Hades::gatherv( Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, Addr recvcnt, Addr displs,
        PayloadDataType recvtype,
        RankID root, Communicator group, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"sendbuf=%p recvbuf=%p sendcnt=%d "
        "recvcntPtr=%p\n", sendbuf,recvbuf,sendcnt,recvcnt);
    m_functionSM->start( new GatherStartEvent(FunctionSM::Gatherv, retFunc,
            sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, displs, recvtype, root, group ) );
}

void Hades::alltoall(
        Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor* retFunc) 
{
    m_dbg.verbose(CALL_INFO,1,0,"sendbuf=%p recvbuf=%p sendcnt=%d "
        "recvcnt=%d\n", sendbuf,recvbuf,sendcnt,recvcnt);
    m_functionSM->start( new AlltoallStartEvent( FunctionSM::Alltoall, retFunc,
            sendbuf,sendcnt, sendtype, recvbuf, recvcnt, recvtype, group) );
}

void Hades::alltoallv(
        Addr sendbuf, Addr sendcnts, Addr senddispls, PayloadDataType sendtype,
        Addr recvbuf, Addr recvcnts, Addr recvdispls, PayloadDataType recvtype,
        Communicator group, Functor* retFunc ) 
{
    m_dbg.verbose(CALL_INFO,1,0,"sendbuf=%p recvbuf=%p sendcntPtr=%p "
        "recvcntPtr=%p\n", sendbuf,recvbuf,sendcnts,recvcnts);
    m_functionSM->start( new AlltoallStartEvent( FunctionSM::Alltoallv, retFunc,
            sendbuf, sendcnts, senddispls, sendtype, 
            recvbuf, recvcnts, recvdispls, recvtype,
            group) );
}

void Hades::barrier(Communicator group, Functor* retFunc)
{
    m_functionSM->start( new BarrierStartEvent(FunctionSM::Barrier,
                         retFunc, group) );
}

void Hades::probe(RankID source, uint32_t tag,
        Communicator group, MessageResponse* resp, Functor* retFunc )
{
}

void Hades::wait(MessageRequest* req, MessageResponse* resp,
        Functor* retFunc )
{
    m_functionSM->start( new WaitStartEvent( FunctionSM::Wait, 
                    retFunc, req, resp) );
}

void Hades::test(MessageRequest* req, int& flag, MessageResponse* resp,
        Functor* retFunc)
{
}
