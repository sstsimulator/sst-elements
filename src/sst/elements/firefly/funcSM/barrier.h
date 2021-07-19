// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_BARRIER_H
#define COMPONENTS_FIREFLY_FUNCSM_BARRIER_H

#include "funcSM/collectiveTree.h"

namespace SST {
namespace Firefly {

class BarrierFuncSM :  public CollectiveTreeFuncSM
{
  public:
    SST_ELI_REGISTER_MODULE(
        BarrierFuncSM,
        "firefly",
        "Barrier",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

  public:
    BarrierFuncSM( SST::Params& params ) : CollectiveTreeFuncSM( params ) {}

    virtual void handleStartEvent( SST::Event* e, Retval& retval ) {
        BarrierStartEvent* event = static_cast<BarrierStartEvent*>( e );

		Hermes::MemAddr addr(1,NULL);
        CollectiveStartEvent* tmp = new CollectiveStartEvent( addr, addr, 0,
                MP::CHAR, MP::MAX, 0, event->group,
                                CollectiveStartEvent::Allreduce );

        delete event;

        CollectiveTreeFuncSM::handleStartEvent(
                        static_cast<SST::Event*>( tmp ), retval );
    }

    virtual void handleEnterEvent( Retval& retval ) {
        CollectiveTreeFuncSM::handleEnterEvent( retval );
    }

    virtual std::string protocolName() { return "CtrlMsgProtocol"; }
};

}
}

#endif
