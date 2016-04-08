// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_COMMSPLIT_H
#define COMPONENTS_FIREFLY_FUNCSM_COMMSPLIT_H

#include "funcSM/allgather.h"

namespace SST {
namespace Firefly {

class CommSplitFuncSM :  public AllgatherFuncSM
{
  public:
    CommSplitFuncSM( SST::Params& params )
        : AllgatherFuncSM( params ), m_commSplitEvent(0) {}

    virtual void handleStartEvent( SST::Event*, Retval& );
    virtual void handleEnterEvent( Retval& );
    
  private:
    int* m_sendbuf; 
    int* m_recvbuf;
    CommSplitStartEvent* m_commSplitEvent;
};

}
}

#endif
