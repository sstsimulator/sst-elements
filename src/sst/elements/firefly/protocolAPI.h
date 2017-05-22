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

#ifndef COMPONENTS_FIREFLY_PROTOCOLAPI_H
#define COMPONENTS_FIREFLY_PROTOCOLAPI_H

#include <sst/core/subcomponent.h>
#include "sst/elements/thornhill/memoryHeapLink.h"

namespace SST {
namespace Firefly {

class Info;
class VirtNic;

class ProtocolAPI : public SST::SubComponent 
{
  public:

    ProtocolAPI( Component* parent ) : SubComponent( parent ) {}
    virtual ~ProtocolAPI() {}
    virtual void printStatus( Output& ) {}
    virtual void setup() {};
    virtual void finish() {};
    virtual void init( Info*, VirtNic*, Thornhill::MemoryHeapLink* ) = 0;  
    virtual std::string name() = 0;
    virtual void setRetLink(SST::Link* link) { assert(0); } 
};

}
}
#endif
