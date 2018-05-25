// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_LOOPBACK_H
#define COMPONENTS_FIREFLY_LOOPBACK_H

#include <sst/core/elementinfo.h>
#include <sst/core/component.h>

namespace SST {
namespace Firefly {

class LoopBackEventBase : public Event {

  public:
    LoopBackEventBase( int _core ) : Event(), core( _core ) {}
    int core;

    NotSerializable(LoopBackEventBase)
};

class LoopBack : public SST::Component  {
  public:
    SST_ELI_REGISTER_COMPONENT(
        LoopBack,
        "firefly",
        "loopBack",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        COMPONENT_CATEGORY_SYSTEM
    )
    SST_ELI_DOCUMENT_PARAMS(
        {"numCores","Sets the number cores to create links to", "1"},
    )
    SST_ELI_DOCUMENT_PORTS(
        {"core%(num_vNics)d", "Ports connected to the network driver", {}}
    )

    LoopBack(ComponentId_t id, Params& params );
    ~LoopBack() {}
    void handleCoreEvent( Event* ev, int );

  private:
    std::vector<Link*>          m_links;   
};

}
}

#endif
