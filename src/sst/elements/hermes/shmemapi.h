// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_HERMES_SHMEM_INTERFACE
#define _H_HERMES_SHMEM_INTERFACE

#include <assert.h>

#include "hermes.h"

using namespace SST;

namespace SST {
namespace Hermes {
namespace Shmem {

typedef Arg_FunctorBase< int, bool > Functor;

class Interface : public Hermes::Interface {
    public:

    Interface( Component* parent ) : Hermes::Interface(parent) {}
    virtual ~Interface() {}
    virtual void setOS( OS* ) { assert(0); }

    virtual void start_pes(int,Functor*) {}
    virtual void num_pes(Functor*) {}
    virtual void my_pe(Functor*) {}
    virtual void malloc(size_t, Functor*) {}
    virtual void free(void*, Functor*) {}
    virtual void realloc(void*, size_t, Functor*) {}
    virtual void memalign(size_t, size_t, Functor*) {}
#if 0
    virtual void p<Type>( Type* target, Type value, int pe, Functor*) {}
    virtual void put<Type>( Type* target, const Type* src, 
                        size_t nelems, int pe, Functor* ) {}
    virtual void putSS( void* target, const Type* src, 
                        size_t nelems, int pe, Functor* ) {}
    virtual void g<Type>( Type* target, int pe, Functor* ) {}
    virtual void get<Type>( Type* target, const Type* src, 
                        size_t nelems, int pe, Functor* ) {}
    virtual void getSS( void* target, const Type* src, 
                        size_t nelems, int pe, Functor* ) {}
#endif
};

}
}
}

#endif
