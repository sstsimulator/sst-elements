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


#ifndef _loadMemory_h
#define _loadMemory_h

#include <sst/core/params.h>
#include <sst/core/output.h> // Must be loaded before any Gem5 includes (overwrites 'fatal')
#include <mem/physical.hh>

namespace SST {
namespace M5 {


void loadMemory( std::string name, ::PhysicalMemory* realMemory, 
                    const SST::Params& params );

}
}
#endif
