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


#ifndef COMPONENTS_PORTALS_DRIVER_H
#define COMPONENTS_PORTALS_DRIVER_H

#include <sst/core/component.h>

namespace SST {
namespace portals {

class driver : public Component {
public:
    driver(ComponentId_t id, Params& params);
};

}
}

#endif 
