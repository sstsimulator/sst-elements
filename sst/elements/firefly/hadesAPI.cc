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
#include "functionCtx.h"
#include "funcCtx/rank.h"
#include "funcCtx/size.h"
#include "funcCtx/send.h"
#include "funcCtx/recv.h"
#include "funcCtx/wait.h"

#define DPRINTF( fmt, args...) __DBG( DBG_NETWORK, testHermes, fmt, ## args )

using namespace SST::Firefly;
using namespace Hermes;

void Hades::init(Functor* retFunc )
{
    setCtx( new FunctionCtx(retFunc, FunctionCtx::Init, this ) );  
}

void Hades::fini(Functor* retFunc)
{
    setCtx( new FunctionCtx(retFunc, FunctionCtx::Fini, this) ); 
}

void Hades::rank(Communicator group, int* rank, 
        Functor* retFunc)
{
    setCtx( new RankCtx( group, rank, retFunc, FunctionCtx::Rank, this ) ); 
}

void Hades::size(Communicator group, int* size,
        Functor* retFunc )
{
    setCtx( new SizeCtx(group, size, retFunc, FunctionCtx::Size, this) ); 
}

void Hades::send(Hermes::Addr buf, uint32_t count, 
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group,
        Functor* retFunc )
{
    setCtx( new SendCtx( buf, count, dtype, dest, tag, group,
                            NULL, retFunc, FunctionCtx::Send, this ) ); 
}

void Hades::isend(Addr buf, uint32_t count, PayloadDataType dtype,
        RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc )
{
    setCtx( new SendCtx( buf, count, dtype, dest, tag, group,
                            req, retFunc, FunctionCtx::Isend, this ) ); 
}

void Hades::recv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageResponse* resp, Functor* retFunc )
{
    setCtx( new RecvCtx( target, count, dtype, source, tag, group,
                            resp, NULL, retFunc, FunctionCtx::Recv, this ) ); 
}

void Hades::irecv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageRequest* req, Functor* retFunc)
{
    setCtx( new RecvCtx( target, count, dtype, source, tag, group,
                            NULL, req, retFunc, FunctionCtx::Irecv, this ) ); 
}

void Hades::allreduce(Addr mydata, Addr result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op,
        Communicator group, Functor* retFunc)
{
}


void Hades::reduce(void* mydata, void* result, uint32_t count,
        PayloadDataType dtype, ReductionOperation op, RankID root,
        Communicator group, Functor* retFunc)
{
}

void Hades::barrier(Communicator group, Functor* retFunc)
{
    setCtx( new FunctionCtx(retFunc, FunctionCtx::Barrier, this) );
}

void Hades::probe(RankID source, uint32_t tag,
        Communicator group, MessageResponse* resp, Functor* retFunc )
{
    
}

void Hades::wait(MessageRequest* req, MessageResponse* resp,
        Functor* retFunc )
{
    setCtx( new WaitCtx( req, resp, retFunc, FunctionCtx::Wait, this) ); 
}

void Hades::test(MessageRequest* req, int& flag, MessageResponse* resp,
        Functor* retFunc)
{
}
