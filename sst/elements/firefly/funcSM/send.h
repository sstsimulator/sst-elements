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

#ifndef COMPONENTS_FIREFLY_FUNCSM_SEND_H
#define COMPONENTS_FIREFLY_FUNCSM_SEND_H

#include "funcSM/api.h"
#include "funcSM/event.h"

namespace SST {
namespace Firefly {

class DataMovement;
class ProtocolAPI;

class SendFuncSM :  public FunctionSMInterface
{
    enum { } m_state;
  public:
    SendFuncSM( int verboseLevel, Output::output_location_t loc,
        Info* info, SST::Link*, ProtocolAPI* );

    virtual void  handleEnterEvent( SST::Event *e);
    virtual void handleProgressEvent( SST::Event* );

    virtual const char* name() {
       return "Send"; 
    }

  private:
    DataMovement*   m_dm;
    SST::Link*      m_toProgressLink;
    SendEnterEvent* m_event;
    Hermes::MessageRequest m_req; 
};

}
}

#endif
