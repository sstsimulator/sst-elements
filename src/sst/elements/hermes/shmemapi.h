// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
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

typedef Hermes::Callback Callback;

typedef int Fam_Descriptor;

typedef enum { LTE, LT, EQ, NE, GT, GTE } WaitOp;

static std::string WaitOpName( WaitOp op ) {
	switch( op ) {
		case LTE:
			return "LTE";
		case LT:
			return "LT";
		case EQ:
			return "EQ";
		case NE:
			return "NE";
		case GT:
			return "GT";
		case GTE:
			return "GTE";
	}
}

typedef enum { MOVE, AND, MAX, MIN, SUM, PROD, OR, XOR } ReduOp;

class Interface : public Hermes::Interface {
    public:

    Interface( ComponentId_t id ) : Hermes::Interface(id) {}
    virtual ~Interface() {}

    virtual void init(Callback) { assert(0); }
    virtual void finalize(Callback) { assert(0); }

    virtual void n_pes(int*, Callback) { assert(0); }
    virtual void my_pe(int*, Callback) { assert(0); }

    virtual void fence(Callback) { assert(0); }
    virtual void quiet(Callback) { assert(0); }

    virtual void barrier_all(Callback) { assert(0); }
    virtual void barrier( int start, int stride, int size, Vaddr, Callback) { assert(0); }
    virtual void broadcast( Vaddr dest, Vaddr source, size_t nelems, int root, int PE_start,
            int logPE_stride, int PE_size, Vaddr, Callback) { assert(0); }
    virtual void fcollect( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
            int logPE_stride, int PE_size, Vaddr, Callback) { assert(0); }
    virtual void collect( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
            int logPE_stride, int PE_size, Vaddr, Callback) { assert(0); }
    virtual void alltoall( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
            int logPE_stride, int PE_size, Vaddr, Callback) { assert(0); }
    virtual void alltoalls( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize,
            int PE_start, int logPE_stride, int PE_size, Vaddr, Callback) { assert(0); }
    virtual void reduction( Vaddr dest, Vaddr source, int nelems, int PE_start,
            int logPE_stride, int PE_size, Vaddr pSync,
            ReduOp, Hermes::Value::Type, Callback) { assert(0); }

    virtual void malloc(MemAddr*, size_t, bool backed, Callback) { assert(0); }
    virtual void free(MemAddr&, Callback) { assert(0); }

    virtual void get( Vaddr dst, Vaddr src, size_t nelems, int pe, bool blocking, Callback) { assert(0); }
    virtual void put( Vaddr dst, Vaddr src, size_t nelems, int pe, bool blocking, Callback) { assert(0); }

    virtual void getv( Value& result, Vaddr src, int pe, Callback) { assert(0); }
    virtual void putv( Vaddr dest, Value& value, int pe, Callback) { assert(0); }

    virtual void wait_until( Vaddr, WaitOp, Value&, Callback) { assert(0); }
    virtual void cswap( Value& result, Vaddr, Value& cond, Value& value, int pe, Callback) { assert(0); }
    virtual void swap( Value& result, Vaddr, Value&, int pe, Callback) { assert(0); }
    virtual void fadd( Value& result, Vaddr, Value&, int pe, Callback) { assert(0); }
    virtual void add( Vaddr, Value&, int pe, Callback) { assert(0); }

    virtual void fam_get( Hermes::Vaddr dest, Fam_Descriptor fd, uint64_t offset, uint64_t nbytes,
			bool blocking, Callback &) { assert(0); }
    virtual void fam_put( Fam_Descriptor fd, uint64_t offset, Hermes::Vaddr dest, uint64_t nbytes,
			bool blocking, Callback& ) { assert(0); }

	virtual void fam_scatter( Hermes::Vaddr src, Fam_Descriptor fd, uint64_t nElements, uint64_t firstElement,
			uint64_t stride, uint64_t elmentSize, bool blocking, Callback& ) {assert(0); };
	virtual void fam_scatterv( Hermes::Vaddr src, Fam_Descriptor fd, uint64_t nElements,
			std::vector<uint64_t> elementIndex, uint64_t elmentSize, bool blocking, Callback& ) {assert(0);};

	virtual void fam_gather( Hermes::Vaddr dest, Fam_Descriptor fd, uint64_t nElements, uint64_t firstElement,
			uint64_t stride, uint64_t elmentSize, bool blocking, Callback& ) {assert(0); };
	virtual void fam_gatherv( Hermes::Vaddr dest, Fam_Descriptor fd, uint64_t nElements,
			std::vector<uint64_t> elementIndex, uint64_t elmentSize, bool blocking, Callback& ) {assert(0);};

    virtual void fam_add( Fam_Descriptor fd, uint64_t, Value&, Callback& ) { assert(0); }
    virtual void fam_cswap( Value& result, Fam_Descriptor fd, uint64_t, Value& oldValue , Value& newValue, Callback) { assert(0); }
};

}
}
}

#endif
