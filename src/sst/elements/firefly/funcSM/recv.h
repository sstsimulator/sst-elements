// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_RECV_H
#define COMPONENTS_FIREFLY_FUNCSM_RECV_H

#include "funcSM/api.h"
#include "funcSM/event.h"
#include "ctrlMsg.h"

namespace SST {
namespace Firefly {

class RecvFuncSM :  public FunctionSMInterface
{
//    enum { Wait, Exit } m_state;

  public:
    RecvFuncSM( SST::Params& params );

    virtual void handleStartEvent( SST::Event*, Retval& );
    virtual void handleEnterEvent( Retval& );

    virtual std::string protocolName() { return "CtrlMsgProtocol"; }

  private:

    RecvStartEvent*		m_event;

    CtrlMsg::API* proto() { return static_cast<CtrlMsg::API*>(m_proto); }
};

}
}

#endif
