// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
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

#include <sst/core/component.h>

namespace SST {
namespace Firefly {

class LoopBackEvent : public Event {

  public:
    LoopBackEvent( int _core ) : Event(), core( _core ) {}
    int core;

    NotSerializable(LoopBackEvent)
};

class LoopBack : public SST::Component  {
  public:
    LoopBack(ComponentId_t id, Params& params );
    ~LoopBack() {}
    void handleCoreEvent( Event* ev, int );

  private:
    std::vector<Link*>          m_links;   
};

}
}

#endif
