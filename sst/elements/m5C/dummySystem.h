// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _dummySystem_h
#define _dummySystem_h

#include <sim/system.hh>
#include <dummyThreadContext.h>
#include <arch/isa_traits.hh>

#include <debug.h>

using namespace std;
using namespace TheISA;

namespace SST {
namespace M5 {

struct DummySystemParams : public SystemParams {
    Addr start;
    Addr end;
};

class DummySystem : public System {

  public:
    typedef DummySystemParams Params;
    DummySystem( Params *p ) :  
        System(p),
        m_start(p->start),
        m_end(p->end) 
    {}

    ~DummySystem() {}

    ThreadContext *getThreadContext(ThreadID tid) { 
        // NOTE: we only have one threadContext 
        // given the thread context does nothing this works 
        return& m_threadContext; 
    }

    Addr allocPhysPages(int npages) {
        Addr return_addr = pagePtr << LogVMPageSize;
        pagePtr += npages;
        if (return_addr >= m_end - m_start)
            fatal("m5C:DummySystem: Out of memory, please increase size of physical memory.");
        return m_start + return_addr;
    }

  private:
    DummyThreadContext m_threadContext;
    Addr m_start;
    Addr m_end;
};

static inline DummySystem* create_DummySystem( string name, 
                    ::PhysicalMemory* physmem,
                    Enums::MemoryMode mem_mode,
                    Addr start, Addr end) 
{
    
    DummySystem::Params& params = *new DummySystem::Params;
    
    DBGC( 1, "%s\n", name.c_str());

    params.name = name;
    params.physmem = physmem;
    params.mem_mode = mem_mode;
    params.start = start;
    params.end = end;

    return new DummySystem( &params );
}

}
}

#endif
