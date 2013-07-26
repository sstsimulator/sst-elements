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
#include "sst/core/serialization/element.h"

#include "hades.h"
#include "functionSM.h"
#include "funcSM/event.h"

using namespace SST::Firefly;
using namespace Hermes;

void Hades::init(Functor* retFunc )
{
    m_functionSM->start( new InitEnterEvent( FunctionSM::Init, retFunc) );
}

void Hades::fini(Functor* retFunc)
{
    m_functionSM->start( new FiniEnterEvent( FunctionSM::Fini, retFunc) );
}

void Hades::rank(Communicator group, int* rank, Functor* retFunc)
{
    m_functionSM->start( new RankEnterEvent(FunctionSM::Rank,
                                        retFunc, group, rank) );
}

void Hades::size(Communicator group, int* size, Functor* retFunc )
{
    m_functionSM->start( new SizeEnterEvent(FunctionSM::Size, 
                                        retFunc, group, size) );
}

void Hades::send(Addr buf, uint32_t count, 
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group,
        Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p count=%d dtype=%d dest=%d tag=%d \
            group=%d\n", buf,count,dtype,dest,tag,group);
    m_functionSM->start( new SendEnterEvent( FunctionSM::Send, 
         retFunc, buf, count, dtype, dest, tag, group, NULL) ); 
}

void Hades::isend(Addr buf, uint32_t count, PayloadDataType dtype,
        RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p count=%d dtype=%d dest=%d tag=%d \
            group=%d\n", buf,count,dtype,dest,tag,group);
    m_functionSM->start( new SendEnterEvent( FunctionSM::Send, 
         retFunc, buf, count, dtype, dest, tag, group, req ) );
}

void Hades::recv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageResponse* resp, Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"target=%p count=%d dtype=%d source=%d tag=%d \
            group=%d\n", target,count,dtype,source,tag,group);
    m_functionSM->start( new RecvEnterEvent( FunctionSM::Recv, 
            retFunc, target, count, dtype, source, tag, group, NULL, resp ) );
}

void Hades::irecv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"target=%p count=%d dtype=%d source=%d tag=%d \
            group=%d\n", target,count,dtype,source,tag,group);
    m_functionSM->start( new RecvEnterEvent( FunctionSM::Recv, 
        retFunc, target, count, dtype, source, tag, group, req, NULL ) );
}

void Hades::allreduce(Addr mydata, Addr result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op,
        Communicator group, Functor* retFunc)
{
    m_dbg.verbose(CALL_INFO,1,0,"in=%p out=%p count=%d dtype=%d \n",
                mydata,result,count,dtype);
    m_functionSM->start( new CollectiveEnterEvent(FunctionSM::Allreduce,
                         retFunc, mydata, result, count, 
                        dtype, op, 0, group, true) );
}

void Hades::reduce(void* mydata, void* result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op, RankID root,
        Communicator group, Functor* retFunc)
{
    m_functionSM->start( new CollectiveEnterEvent(FunctionSM::Reduce,
                         retFunc, mydata, result, count, 
                        dtype, op, root, group, false) );
}

void Hades::barrier(Communicator group, Functor* retFunc)
{
    m_functionSM->start( new BarrierEnterEvent(FunctionSM::Barrier,
                         retFunc, group) );
}

void Hades::probe(RankID source, uint32_t tag,
        Communicator group, MessageResponse* resp, Functor* retFunc )
{
}

void Hades::wait(MessageRequest* req, MessageResponse* resp,
        Functor* retFunc )
{
    m_functionSM->start( new WaitEnterEvent( FunctionSM::Wait, 
                    retFunc, req, resp) );
}

void Hades::test(MessageRequest* req, int& flag, MessageResponse* resp,
        Functor* retFunc)
{
}
