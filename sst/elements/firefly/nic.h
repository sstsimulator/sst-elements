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


#ifndef COMPONENTS_FIREFLY_NIC_H 
#define COMPONENTS_FIREFLY_NIC_H 

#include <sst/core/sst_types.h>
#include <sst/core/component.h>

namespace SST {
namespace Firefly {

class Nic : public SST::Component {
  public:
    Nic(SST::ComponentId_t id, SST::Params &params);
    ~Nic();
    void setup(void) { }
    void init(unsigned int phase) { }
  private:
};

} // namesapce Firefly 
} // namespace SST

#endif
