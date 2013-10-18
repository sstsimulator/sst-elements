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

namespace SST {
namespace Firefly {

class DataMovement;
class Info;
class MsgEntry;
class ProtocolAPI;

class RecvFuncSM :  public FunctionSMInterface
{
    enum { WaitMatch, WaitCopy } m_state;
  public:

    RecvFuncSM( int verboseLevel, Output::output_location_t loc,
                                    Info*, ProtocolAPI* );

    virtual void handleStartEvent( SST::Event*, Retval& );
    virtual void handleEnterEvent( SST::Event*, Retval& );
    virtual void handleSelfEvent( SST::Event*, Retval& );

    virtual const char* name() {
       return "Recv"; 
    }

  private:
    void finish( RecvEntry*, MsgEntry* );
    
    DataMovement*   m_dm;
    RecvStartEvent* m_event;
    MsgEntry*       m_entry;
};

}
}

#endif
