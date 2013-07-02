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
typedef uint32_t Communicator;
typedef uint32_t RankID;

typedef struct MessageResponse {
    uint32_t    tag;  
    RankID      src; 
} MessageResponse;

typedef struct MessageRequest {
    uint32_t    tag;  
    RankID      src; 
} MessageRequest;

enum PayloadDataType {
        CHAR,
	INT,
	LONG,
	DOUBLE,
	FLOAT,
	COMPLEX
};

enum ReductionOperation {
	SUM,
	MIN,
	MAX
};

typedef Arg_FunctorBase< int > Functor; 

static const uint32_t AnyTag = -1;
static const uint32_t AnySrc = -1;
static const Communicator GroupWorld = 0;

enum Retval {
    SUCCESS,
    FAILURE
};

class MessageInterface : public Module {
    public:

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

    MessageInterface() {}
    virtual ~MessageInterface() {}

    virtual void _componentInit(unsigned int phase ) {}
    virtual void _componentSetup( void ) {}
    virtual RankID myWorldRank() {}
    virtual void init(Functor*) {}
    virtual void fini(Functor*) {}
    virtual void rank(Communicator group, int* rank, Functor*) {}
    virtual void size(Communicator group, int* size, Functor*) {}

    virtual void send(Addr payload, uint32_t count, PayloadDataType dtype, 
        RankID dest, uint32_t tag, Communicator group, 
        Functor* ) {}

    virtual void isend(Addr payload, uint32_t count, PayloadDataType dtype, 
        RankID dest, uint32_t tag, Communicator group, 
        MessageRequest* req, Functor* ) {}

    virtual void recv(Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group, 
        MessageResponse* resp, Functor*) {}

    virtual void irecv(Addr target, uint32_t count, PayloadDataType dtype, 
        RankID source, uint32_t tag, Communicator group, 
        MessageRequest* req, Functor*) {}

    virtual void allreduce(Addr mydata, Addr result, uint32_t count, 
        PayloadDataType dtype, ReductionOperation op, 
        Communicator group, Functor*) {}

    virtual void reduce(Addr mydata, Addr result, uint32_t count, 
        PayloadDataType dtype, ReductionOperation op, RankID root, 
        Communicator group, Functor*) {}

    virtual void barrier(Communicator group, Functor*) {}

    virtual int probe( RankID source, uint32_t tag, 
        Communicator group, MessageResponse* resp, Functor* ) {} 

    virtual int wait( MessageRequest* req, MessageResponse* resp,
        Functor* ) {};

    virtual int test( MessageRequest* req, int& flag, MessageResponse* resp,
        Functor* ) {};
    
};

}
}

#endif

