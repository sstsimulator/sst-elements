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

namespace SST {
namespace Hermes {
namespace Shmem {

typedef Arg_FunctorBase< int, bool > Functor;
typedef enum { NONE, LTE, LT, E, NE, GT, GTE } WaitOp;
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
    virtual void free(MemAddr&, Functor*) { assert(0); }

    virtual void get( Vaddr dst, Vaddr src, size_t nelems, int pe, Functor*) { assert(0); }
    virtual void put( Vaddr dst, Vaddr src, size_t nelems, int pe, Functor*) { assert(0); }

    virtual void getv( Value& result, Vaddr src, int pe, Functor*) { assert(0); }
    virtual void putv( Vaddr dest, Value& value, int pe, Functor*) { assert(0); }

    virtual void wait_until( Vaddr, WaitOp, Value&, Functor*) { assert(0); }
    virtual void cswap( Value& result, Vaddr, Value& cond, Value& value, int pe, Functor*) { assert(0); }
    virtual void swap( Value& result, Vaddr, Value&, int pe, Functor*) { assert(0); }
    virtual void fadd( Value& result, Vaddr, Value&, int pe, Functor*) { assert(0); }
};

}
}
}

#endif
