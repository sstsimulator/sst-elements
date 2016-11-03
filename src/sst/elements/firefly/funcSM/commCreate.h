// Copyright 2013-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_COMMCREATE_H
#define COMPONENTS_FIREFLY_FUNCSM_COMMCREATE_H

#include "funcSM/barrier.h"

namespace SST {
namespace Firefly {

class CommCreateFuncSM :  public BarrierFuncSM
{
  public:
    CommCreateFuncSM( SST::Params& params )
        : BarrierFuncSM( params ) {}

    virtual void handleStartEvent( SST::Event*, Retval& );
    virtual void handleEnterEvent( Retval& );
    
  private:
};

}
}

#endif
