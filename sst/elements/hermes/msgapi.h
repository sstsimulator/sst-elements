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

#ifndef _H_HERMES_MESSAGE_INTERFACE
#define _H_HERMES_MESSAGE_INTERFACE

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/module.h>

#include <assert.h>
#include "functor.h"

using namespace SST;

namespace SST {
namespace Hermes {

typedef void*    Addr;
typedef uint32_t HermesCommunicator;
typedef uint32_t HermesRankID;

typedef struct MessageResponse {
} MessageResponse;

typedef struct MessageRequest {
    uint32_t     tag;  
    HermesRankID src; 
} MessageRequest;

class MessageInterface : public Module {
    public:

    typedef Arg_FunctorBase< int > AbstractFunctor; 

    enum PayloadDataType {
        CHAR,
	INT,
	LONG,
	DOUBLE,
	FLOAT,
	COMPLEX
    };

    unsigned sizeofDataType( PayloadDataType type ) {
        switch( type ) {
            case CHAR:
                return sizeof( char ); 
            case INT:
                return sizeof( int ); 
            case LONG:
                return sizeof( long ); 
            case DOUBLE:
                return sizeof( double); 
            case FLOAT:
                return sizeof( float ); 
        }
        assert( 0 );
    }

    enum ReductionOperation {
	SUM,
	MIN,
	MAX
    };

    static const uint32_t AnyTag = -1;
    static const uint32_t AnySrc = -1;

    enum Retval {
        SUCCESS
    };

    MessageInterface() {}
    virtual ~MessageInterface() {}

    virtual void setOwningComponent(SST::Component* owner) {}

    virtual void init(AbstractFunctor*) {}
    virtual void fini(AbstractFunctor*) {}
    virtual void rank(HermesCommunicator group, int* rank, AbstractFunctor*) {}
    virtual void size(HermesCommunicator group, AbstractFunctor* ) {}

    virtual void send(Addr payload, uint32_t count, PayloadDataType dtype, 
        HermesRankID dest, uint32_t tag, HermesCommunicator group, 
        AbstractFunctor* ) {}

    virtual void isend(Addr payload, uint32_t count, PayloadDataType dtype, 
        HermesRankID dest, uint32_t tag, HermesCommunicator group, 
        MessageRequest* req, AbstractFunctor* ) {}

    virtual void recv(Addr target, uint32_t count, PayloadDataType dtype,
        HermesRankID source, uint32_t tag, HermesCommunicator group, 
        MessageResponse* resp, AbstractFunctor*) {}

    virtual void irecv(Addr target, uint32_t count, PayloadDataType dtype, 
        HermesRankID source, uint32_t tag, HermesCommunicator group, 
        MessageRequest* req, AbstractFunctor*) {}

    virtual void allreduce(Addr mydata, Addr result, uint32_t count, 
        PayloadDataType dtype, ReductionOperation op, 
        HermesCommunicator group, AbstractFunctor*) {}

    virtual void reduce(Addr mydata, Addr result, uint32_t count, 
        PayloadDataType dtype, ReductionOperation op, HermesRankID root, 
        HermesCommunicator group, AbstractFunctor*) {}

    virtual void barrier(HermesCommunicator group, AbstractFunctor*) {}

    virtual int probe( HermesRankID source, uint32_t tag, 
        HermesCommunicator group, MessageResponse* resp, AbstractFunctor* ) {} 

    virtual int wait( MessageRequest* req, MessageResponse* resp,
        AbstractFunctor* ) {};

    virtual int test( MessageRequest* req, int& flag, MessageResponse* resp,
        AbstractFunctor* ) {};
    
};

}
}

#endif

