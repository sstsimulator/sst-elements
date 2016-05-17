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

#include <sst_config.h>

#include "hadesMP.h"
#include "funcSM/event.h"

using namespace SST::Firefly;
using namespace Hermes;
using namespace Hermes::MP;

HadesMP::HadesMP(Component* owner, Params& params) :
	m_os(NULL)
{
    Params tmpParams = params.find_prefix_params("ctrlMsg.");
	m_proto =
        dynamic_cast<ProtocolAPI*>(owner->loadModuleWithComponent(
                            "firefly.CtrlMsgProto", owner, tmpParams ) );

	Params funcParams = params.find_prefix_params("functionSM.");

	m_functionSM = new FunctionSM( funcParams, owner, m_proto );
}

HadesMP::~HadesMP() {
	delete m_proto;
    delete m_functionSM;
}

void HadesMP::setup( )
{
	assert(m_os);
	m_proto->init( m_os->getInfo(), m_os->getNic() );
	m_proto->setup( );
    m_functionSM->setup(m_os->getInfo() );
}

void HadesMP::finish(  )
{
    m_proto->finish();
}

#if 0
void HadesMP::printStatus( Output& out )
{
    std::map<int,ProtocolAPI*>::iterator iter= m_protocolM.begin();
    for ( ; iter != m_protocolM.end(); ++iter ) {
        iter->second->printStatus(out);
    }
    m_functionSM->printStatus( out );
}
#endif

void HadesMP::init(Functor* retFunc )
{
    functionSM().start( FunctionSM::Init, retFunc, new InitStartEvent() );
}

void HadesMP::fini(Functor* retFunc)
{
    functionSM().start( FunctionSM::Fini, retFunc, new FiniStartEvent() );
}

void HadesMP::rank(Communicator group, RankID* rank, Functor* retFunc)
{
    functionSM().start(FunctionSM::Rank, retFunc,
                            new RankStartEvent( group, (int*) rank) );
}

void HadesMP::size(Communicator group, int* size, Functor* retFunc )
{
    functionSM().start( FunctionSM::Size, retFunc,
                            new SizeStartEvent( group, size) );
}

void HadesMP::send(Addr buf, uint32_t count, 
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group,
        Functor* retFunc )
{
   dbg().verbose(CALL_INFO,1,1,"buf=%p count=%d dtype=%d dest=%d tag=%d "
                        "group=%d\n", buf,count,dtype,dest,tag,group);
    functionSM().start( FunctionSM::Send, retFunc,
            new SendStartEvent( buf, count, dtype, dest, tag, group, NULL) ); 
}

void HadesMP::isend(Addr buf, uint32_t count, PayloadDataType dtype,
        RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc )
{
    dbg().verbose(CALL_INFO,1,1,"buf=%p count=%d dtype=%d dest=%d tag=%d "
                        "group=%d\n", buf,count,dtype,dest,tag,group);
    functionSM().start( FunctionSM::Send, retFunc, 
                new SendStartEvent( buf, count, dtype, dest, tag, group, req));
}

void HadesMP::recv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageResponse* resp, Functor* retFunc )
{
    dbg().verbose(CALL_INFO,1,1,"target=%p count=%d dtype=%d source=%d tag=%d "
                        "group=%d\n", target,count,dtype,source,tag,group);
    functionSM().start( FunctionSM::Recv, retFunc,
      new RecvStartEvent(target, count, dtype, source, tag, group, NULL, resp));
}

void HadesMP::irecv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc)
{
    dbg().verbose(CALL_INFO,1,1,"target=%p count=%d dtype=%d source=%d tag=%d " 
                        "group=%d\n", target,count,dtype,source,tag,group);
    functionSM().start( FunctionSM::Recv, retFunc,
      new RecvStartEvent(target, count, dtype, source, tag, group, req, NULL));
}

void HadesMP::allreduce(Addr mydata, Addr result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op,
        Communicator group, Functor* retFunc)
{
    dbg().verbose(CALL_INFO,1,1,"in=%p out=%p count=%d dtype=%d\n",
                mydata,result,count,dtype);
    functionSM().start( FunctionSM::Allreduce, retFunc,
    new CollectiveStartEvent(mydata, result, count, dtype, op, 0, group, 
                            CollectiveStartEvent::Allreduce));
}

void HadesMP::reduce(Addr mydata, Addr result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op, RankID root,
        Communicator group, Functor* retFunc)
{
    dbg().verbose(CALL_INFO,1,1,"in=%p out=%p count=%d dtype=%d \n",
                mydata,result,count,dtype);
    functionSM().start( FunctionSM::Reduce, retFunc,
        new CollectiveStartEvent(mydata, result, count, 
                        dtype, op, root, group, 
                            CollectiveStartEvent::Reduce) );
}

void HadesMP::bcast(Addr mydata, uint32_t count,
        PayloadDataType dtype, RankID root,
        Communicator group, Functor* retFunc)
{
    dbg().verbose(CALL_INFO,1,1,"in=%p ount=%d dtype=%d \n",
                mydata,count,dtype);
    functionSM().start( FunctionSM::Reduce, retFunc,
        new CollectiveStartEvent(mydata, NULL, count, 
                        dtype, NOP, root, group, 
                            CollectiveStartEvent::Bcast) );
}

void HadesMP::allgather( Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor* retFunc)
{
    dbg().verbose(CALL_INFO,1,1,"\n");

    functionSM().start( FunctionSM::Allgather, retFunc,
        new GatherStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, recvtype, group ) );
}

void HadesMP::allgatherv(
        Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, Addr recvcnt, Addr displs, PayloadDataType recvtype,
        Communicator group, Functor* retFunc)
{
    dbg().verbose(CALL_INFO,1,1,"sendbuf=%p recvbuf=%p sendcnt=%d "
                    "recvcntPtr=%p\n", sendbuf,recvbuf,sendcnt,recvcnt);
    functionSM().start( FunctionSM::Allgatherv, retFunc,
        new GatherStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, displs, recvtype, group ) );
}

void HadesMP::gather( Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
        RankID root, Communicator group, Functor* retFunc)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start( FunctionSM::Gather, retFunc,
        new GatherStartEvent(sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, recvtype, root, group ) );
}

void HadesMP::gatherv( Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, Addr recvcnt, Addr displs,
        PayloadDataType recvtype,
        RankID root, Communicator group, Functor* retFunc)
{
    dbg().verbose(CALL_INFO,1,1,"sendbuf=%p recvbuf=%p sendcnt=%d "
                    "recvcntPtr=%p\n", sendbuf,recvbuf,sendcnt,recvcnt);
    functionSM().start( FunctionSM::Gatherv, retFunc,
         new GatherStartEvent( sendbuf, sendcnt, sendtype,
            recvbuf, recvcnt, displs, recvtype, root, group ) );
}

void HadesMP::alltoall(
        Addr sendbuf, uint32_t sendcnt, PayloadDataType sendtype,
        Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
        Communicator group, Functor* retFunc) 
{
    dbg().verbose(CALL_INFO,1,1,"sendbuf=%p recvbuf=%p sendcnt=%d "
                        "recvcnt=%d\n", sendbuf,recvbuf,sendcnt,recvcnt);
    functionSM().start( FunctionSM::Alltoall, retFunc,
        new AlltoallStartEvent( sendbuf,sendcnt, sendtype, recvbuf,
                                    recvcnt, recvtype, group) );
}

void HadesMP::alltoallv(
        Addr sendbuf, Addr sendcnts, Addr senddispls, PayloadDataType sendtype,
        Addr recvbuf, Addr recvcnts, Addr recvdispls, PayloadDataType recvtype,
        Communicator group, Functor* retFunc ) 
{
    dbg().verbose(CALL_INFO,1,1,"sendbuf=%p recvbuf=%p sendcntPtr=%p "
                        "recvcntPtr=%p\n", sendbuf,recvbuf,sendcnts,recvcnts);
    functionSM().start( FunctionSM::Alltoallv, retFunc,
        new AlltoallStartEvent( sendbuf, sendcnts, senddispls, sendtype, 
            recvbuf, recvcnts, recvdispls, recvtype,
            group) );
}

void HadesMP::barrier(Communicator group, Functor* retFunc)
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start( FunctionSM::Barrier, retFunc,
                            new BarrierStartEvent( group) );
}

void HadesMP::probe(RankID source, uint32_t tag,
        Communicator group, MessageResponse* resp, Functor* retFunc )
{
}

void HadesMP::wait( MessageRequest req, MessageResponse* resp,
        Functor* retFunc )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start( FunctionSM::Wait, retFunc,
                             new WaitStartEvent( req, resp) );
}

void HadesMP::waitany( int count, MessageRequest req[], int* index,
                            MessageResponse* resp, Functor* retFunc )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start( FunctionSM::WaitAny, retFunc,
                             new WaitAnyStartEvent( count, req, index, resp) );
}

void HadesMP::waitall( int count, MessageRequest req[],
                            MessageResponse* resp[], Functor* retFunc )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start( FunctionSM::WaitAll, retFunc,
                             new WaitAllStartEvent( count, req, resp) );
}

void HadesMP::test(MessageRequest req, int& flag, MessageResponse* resp,
        Functor* retFunc)
{
}

void HadesMP::comm_split( Communicator oldComm, int color, int key,
                            Communicator* newComm, Functor* retFunc )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start( FunctionSM::CommSplit, retFunc,
                       new CommSplitStartEvent( oldComm, color, key, newComm) );
} 

void HadesMP::comm_create( MP::Communicator oldComm, size_t nRanks, int* ranks,
        MP::Communicator* newComm, MP::Functor* retFunc )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start( FunctionSM::CommCreate, retFunc,
            new CommCreateStartEvent( oldComm, nRanks, ranks, newComm) );
}

void HadesMP::comm_destroy( MP::Communicator comm, MP::Functor* retFunc )
{
    dbg().verbose(CALL_INFO,1,1,"\n");
    functionSM().start( FunctionSM::CommDestroy, retFunc,
            new CommDestroyStartEvent( comm ) );
}
