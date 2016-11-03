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
    InitFuncSM( SST::Params& params ) : FunctionSMInterface( params ), m_event(NULL) {}

	virtual std::string protocolName() { return "CtrlMsgProtocol"; }

    virtual void handleStartEvent( SST::Event *e, Retval& retval ) {

		assert( NULL == m_event );
        m_dbg.verbose(CALL_INFO,1,0,"\n");

		m_event = static_cast<InitStartEvent*>(e);

        proto()->init( );
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
