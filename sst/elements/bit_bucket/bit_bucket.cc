// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/cpunicEvent.h>
#include "bit_bucket.h"



void
Bit_bucket::handle_events(Event *event)
{

    CPUNicEvent *e= static_cast<CPUNicEvent *>(event);
    delete(e);

}  // end of handle_events()



extern "C" {
Bit_bucket *
bit_bucketAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Bit_bucket(id, params);
}
}

BOOST_CLASS_EXPORT(Bit_bucket)
