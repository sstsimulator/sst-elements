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

#ifndef COMPONENTS_FIREFLY_FUNCSM_BARRIER_H
#define COMPONENTS_FIREFLY_FUNCSM_BARRIER_H

#include "funcSM/collectiveTree.h"

namespace SST {
namespace Firefly {

class BarrierFuncSM :  public CollectiveTreeFuncSM 
{
  public:
    BarrierFuncSM( int verboseLevel, Output::output_location_t loc,
        Info* info, SST::Link* progressLink, ProtocolAPI* api, SST::Link* );

    virtual void handleEnterEvent( SST::Event *e);
    void handleProgressEvent( SST::Event *e );

    virtual const char* name() {
       return "Barrier"; 
    }
};

}
}

#endif
