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

#ifndef COMPONENTS_FIREFLY_FUNCSM_WAIT_H
#define COMPONENTS_FIREFLY_FUNCSM_WAIT_H

#include <sst/core/output.h>

#include "info.h"
#include "funcSM/api.h"
#include "funcSM/event.h"
#include "ioapi.h"

namespace SST {
namespace Firefly {

class ProtocolAPI;
class DataMovement;

class WaitFuncSM :  public FunctionSMInterface
{
  public:
    WaitFuncSM( int verboseLevel, Output::output_location_t loc,
        Info*, SST::Link*&, ProtocolAPI*, IO::Interface* );

    virtual void handleEnterEvent( SST::Event *e );
    virtual void handleProgressEvent( SST::Event* );
    
    virtual const char* name() {
       return "Wait"; 
    }

  private:

    DataMovement*    m_dm;
    SST::Link*& m_toProgressLink;
    WaitEnterEvent* m_event;
    IO::Interface* m_io;
};

}
}

#endif
