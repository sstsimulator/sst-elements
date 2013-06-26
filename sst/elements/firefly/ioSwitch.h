// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_FIREFLY_IOSWITCH_H 
#define COMPONENTS_FIREFLY_IOSWITCH_H 

#include <sst/core/sst_types.h>
#include <sst/core/component.h>

namespace SST {
namespace Firefly {

class IOSwitch : public SST::Component {
  public:
    IOSwitch(SST::ComponentId_t id, SST::Component::Params_t &params);
    ~IOSwitch();
    void setup(void) { }
    void init(unsigned int phase);

  private:
    void handleEvent(SST::Event*,int);

    std::vector<SST::Link*> m_links;
};

} // namesapce Firefly 
} // namespace SST

#endif
