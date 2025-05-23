// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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
    ProtocolAPI( ComponentId_t id ) : SubComponent( id ) {}
    virtual ~ProtocolAPI() {}
    virtual void setVars( Info*, VirtNic*, Thornhill::MemoryHeapLink*, Link* ) = 0;
    virtual std::string name() = 0;
};

}
}
#endif
