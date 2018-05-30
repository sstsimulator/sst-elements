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

#ifndef COMPONENTS_FIREFLY_FUNCSM_FINI_H
#define COMPONENTS_FIREFLY_FUNCSM_FINI_H

#include "funcSM/barrier.h"

namespace SST {
namespace Firefly {

class FiniFuncSM :  public BarrierFuncSM 
{
 public:
    SST_ELI_REGISTER_MODULE(
        FiniFuncSM,
        "firefly",
        "Fini",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )
  public:
    FiniFuncSM( SST::Params& params ) : BarrierFuncSM( params ) {}

    virtual void handleStartEvent( SST::Event* e, Retval& retval) {

        FiniStartEvent* event = static_cast<FiniStartEvent*>( e );
        BarrierStartEvent* tmp = new BarrierStartEvent( MP::GroupWorld  );

        delete event;

        BarrierFuncSM::handleStartEvent(static_cast<SST::Event*>(tmp), retval );
    }

    virtual void handleEnterEvent( Retval& retval ) {
        BarrierFuncSM::handleEnterEvent( retval );
    }

    virtual std::string protocolName() { return "CtrlMsgProtocol"; }
};

}
}

#endif
