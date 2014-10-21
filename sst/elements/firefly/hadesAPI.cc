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

#include <sst_config.h>

#include "hades.h"
#include "functionSM.h"
#include "funcSM/event.h"

using namespace SST::Firefly;
using namespace Hermes;

void Hades::init(Functor* retFunc )
{
    m_functionSM->start( FunctionSM::Init, retFunc, new InitStartEvent() );
}

void Hades::fini(Functor* retFunc)
{
    m_functionSM->start( FunctionSM::Fini, retFunc, new FiniStartEvent() );
}

void Hades::rank(Communicator group, RankID* rank, Functor* retFunc)
{
    m_functionSM->start(FunctionSM::Rank, retFunc,
                            new RankStartEvent( group, (int*) rank) );
}

void Hades::size(Communicator group, int* size, Functor* retFunc )
{
    m_functionSM->start( FunctionSM::Size, retFunc,
                            new SizeStartEvent( group, size) );
}

void Hades::send(Addr buf, uint32_t count, 
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group,
        Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p count=%d dtype=%d dest=%d tag=%d "
                        "group=%d\n", buf,count,dtype,dest,tag,group);
    m_functionSM->start( FunctionSM::Send, retFunc,
            new SendStartEvent( buf, count, dtype, dest, tag, group, NULL) ); 
}

void Hades::isend(Addr buf, uint32_t count, PayloadDataType dtype,
        RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p count=%d dtype=%d dest=%d tag=%d "
                        "group=%d\n", buf,count,dtype,dest,tag,group);
    m_functionSM->start( FunctionSM::Send, retFunc, 
                new SendStartEvent( buf, count, dtype, dest, tag, group, req));
}

void Hades::recv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageResponse* resp, Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"target=%p count=%d dtype=%d source=%d tag=%d "
                        "group=%d\n", target,count,dtype,source,tag,group);
    m_functionSM->start( FunctionSM::Recv, retFunc,
      new RecvStartEvent(target, count, dtype, source, tag, group, NULL, resp));
}

void Hades::irecv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"target=%p count=%d dtype=%d source=%d tag=%d " 
                        "group=%d\n", target,count,dtype,source,tag,group);
    m_functionSM->start( FunctionSM::Recv, retFunc,
      new RecvStartEvent(target, count, dtype, source, tag, group, req, NULL));
}

void Hades::allreduce(Addr mydata, Addr result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op,
        Communicator group, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"in=%p out=%p count=%d dtype=%d\n",
                mydata,result,count,dtype);
    m_functionSM->start( FunctionSM::Allreduce, retFunc,
    new CollectiveStartEvent(mydata, result, count, dtype, op, 0, group, true));
}

void Hades::reduce(Addr mydata, Addr result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op, RankID root,
        Communicator group, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"in=%p out=%p count=%d dtype=%d \n",
                mydata,result,count,dtype);
    m_functionSM->start( FunctionSM::Reduce, retFunc,
        new CollectiveStartEvent(mydata, result, count, 
                        dtype, op, root, group, false) );
}

void Hades::allgather( Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor* retFunc)
{
    m_functionSM->start( FunctionSM::Allgather, retFunc,
        new GatherStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, recvtype, group ) );
}

void Hades::allgatherv(
        Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, Addr recvcnt, Addr displs, PayloadDataType recvtype,
        Communicator group, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"sendbuf=%p recvbuf=%p sendcnt=%d "
                    "recvcntPtr=%p\n", sendbuf,recvbuf,sendcnt,recvcnt);
    m_functionSM->start( FunctionSM::Allgatherv, retFunc,
        new GatherStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, displs, recvtype, group ) );
}

void Hades::gather( Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
        RankID root, Communicator group, Functor* retFunc)
{
    m_functionSM->start( FunctionSM::Gather, retFunc,
        new GatherStartEvent(sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, recvtype, root, group ) );
}

void Hades::gatherv( Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, Addr recvcnt, Addr displs,
        PayloadDataType recvtype,
        RankID root, Communicator group, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"sendbuf=%p recvbuf=%p sendcnt=%d "
                    "recvcntPtr=%p\n", sendbuf,recvbuf,sendcnt,recvcnt);
    m_functionSM->start( FunctionSM::Gatherv, retFunc,
         new GatherStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, displs, recvtype, root, group ) );
}

void Hades::alltoall(
        Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor* retFunc) 
{
    m_dbg.verbose(CALL_INFO,1,0,"sendbuf=%p recvbuf=%p sendcnt=%d "
                        "recvcnt=%d\n", sendbuf,recvbuf,sendcnt,recvcnt);
    m_functionSM->start( FunctionSM::Alltoall, retFunc,
        new AlltoallStartEvent( sendbuf,sendcnt, sendtype, recvbuf,
                                    recvcnt, recvtype, group) );
}

void Hades::alltoallv(
        Addr sendbuf, Addr sendcnts, Addr senddispls, PayloadDataType sendtype,
        Addr recvbuf, Addr recvcnts, Addr recvdispls, PayloadDataType recvtype,
        Communicator group, Functor* retFunc ) 
{
    m_dbg.verbose(CALL_INFO,1,0,"sendbuf=%p recvbuf=%p sendcntPtr=%p "
                        "recvcntPtr=%p\n", sendbuf,recvbuf,sendcnts,recvcnts);
    m_functionSM->start( FunctionSM::Alltoallv, retFunc,
        new AlltoallStartEvent( sendbuf, sendcnts, senddispls, sendtype, 
            recvbuf, recvcnts, recvdispls, recvtype,
            group) );
}

void Hades::barrier(Communicator group, Functor* retFunc)
{
    m_functionSM->start( FunctionSM::Barrier, retFunc,
                            new BarrierStartEvent( group) );
}

void Hades::probe(RankID source, uint32_t tag,
        Communicator group, MessageResponse* resp, Functor* retFunc )
{
}

void Hades::wait( MessageRequest req, MessageResponse* resp,
        Functor* retFunc )
{
    m_functionSM->start( FunctionSM::Wait, retFunc,
                             new WaitStartEvent( req, resp) );
}

void Hades::waitany( int count, MessageRequest req[], int* index,
                            MessageResponse* resp, Functor* retFunc )
{
    m_functionSM->start( FunctionSM::WaitAny, retFunc,
                             new WaitAnyStartEvent( count, req, index, resp) );
}

void Hades::waitall( int count, MessageRequest req[],
                            MessageResponse* resp[], Functor* retFunc )
{
    m_functionSM->start( FunctionSM::WaitAll, retFunc,
                             new WaitAllStartEvent( count, req, resp) );
}

void Hades::test(MessageRequest req, int& flag, MessageResponse* resp,
        Functor* retFunc)
{
}

void Hades::comm_split( Communicator oldComm, int color, int key,
                            Communicator* newComm, Functor* retFunc )
{
    m_functionSM->start( FunctionSM::CommSplit, retFunc,
                       new CommSplitStartEvent( oldComm, color, key, newComm) );
} 

