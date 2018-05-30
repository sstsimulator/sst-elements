// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_INIT_H
#define COMPONENTS_FIREFLY_FUNCSM_INIT_H

#include "funcSM/api.h"
#include "funcSM/event.h"
#include "ctrlMsg.h"

namespace SST {
namespace Firefly {

class InitFuncSM :  public FunctionSMInterface 
{
  public:
    SST_ELI_REGISTER_MODULE(
        InitFuncSM,
        "firefly",
        "Init",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

  public:
    InitFuncSM( SST::Params& params ) : FunctionSMInterface( params ), m_event(NULL) {}

	virtual std::string protocolName() { return "CtrlMsgProtocol"; }

    virtual void handleStartEvent( SST::Event *e, Retval& retval ) {

		assert( NULL == m_event );
        m_dbg.debug(CALL_INFO,1,0,"\n");

		m_event = static_cast<InitStartEvent*>(e);

        proto()->initMsgPassing( );
    }

    void handleEnterEvent( Retval& retval ) {
        delete m_event;
        m_event = NULL;
        retval.setExit(0);

    }
  private:

    CtrlMsg::API* proto() { return static_cast<CtrlMsg::API*>(m_proto); }

    InitStartEvent* m_event;
		
};

}
}

#endif
