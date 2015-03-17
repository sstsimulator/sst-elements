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

#ifndef _system_h
#define _system_h

#include <sim/system.hh>

namespace SST {
namespace M5 {

static inline System* create_System( std::string name, 
                    PhysicalMemory* physmem,
                    Enums::MemoryMode mem_mode) 
{
    
    System::Params& params = *new System::Params;
    
    params.name = name;
    params.physmem = physmem;
    params.mem_mode = mem_mode;
    params.memories.resize(1);
    params.memories[0] = physmem;

    return new System( &params );
}

}
}

#endif
