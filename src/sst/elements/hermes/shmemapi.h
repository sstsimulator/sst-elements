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

    virtual void init(Functor*) { assert(0); }
    virtual void finalize(Functor*) { assert(0); }

    virtual void barrier_all(Functor*) { assert(0); }
    virtual void fence(Functor*) { assert(0); }

    virtual void n_pes(int*, Functor*) { assert(0); }
    virtual void my_pe(int*, Functor*) { assert(0); }

    virtual void malloc(MemAddr*, size_t, Functor*) { assert(0); }
    virtual void free(MemAddr, Functor*) { assert(0); }

    virtual void get( MemAddr dst, MemAddr src, size_t nelems, int pe, Functor*) { assert(0); }
    virtual void put( MemAddr dst, MemAddr src, size_t nelems, int pe, Functor*) { assert(0); }
    virtual void getv( void*, MemAddr, int size, int pe, Functor*) { assert(0); }
    virtual void putv( MemAddr, uint64_t value, int size, int pe, Functor*) { assert(0); }

    virtual void uint64_gut( MemAddr, MemAddr, int pe, Functor*) { assert(0); }

    virtual void uint64_add(uint64_t* addr, int pe, Functor*) { assert(0); }
};

}
}
}

#endif
