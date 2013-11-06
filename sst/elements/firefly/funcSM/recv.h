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

#ifndef COMPONENTS_FIREFLY_FUNCSM_RECV_H
#define COMPONENTS_FIREFLY_FUNCSM_RECV_H

#include "funcSM/api.h"
#include "funcSM/event.h"
#include "dataMovement.h"

namespace SST {
namespace Firefly {

class RecvFuncSM :  public FunctionSMInterface
{
    enum { Wait, Exit } m_state;

  public:
    RecvFuncSM( SST::Params& params );

    virtual void handleStartEvent( SST::Event*, Retval& );
    virtual void handleEnterEvent( Retval& );

    virtual std::string protocolName() { return "DataMovement"; }

  private:

    DataMovement* proto() { return static_cast<DataMovement*>(m_proto); }
    
    RecvStartEvent*         m_event;
};

}
}

#endif
