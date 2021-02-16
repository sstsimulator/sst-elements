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

#ifndef COMPONENTS_FIREFLY_FUNCSM_ALLREDUCE_H
#define COMPONENTS_FIREFLY_FUNCSM_ALLREDUCE_H

#include "funcSM/collectiveTree.h"

namespace SST {
namespace Firefly {

class AllreduceFuncSM :  public CollectiveTreeFuncSM
{
  public:
    SST_ELI_REGISTER_MODULE(
        AllreduceFuncSM,
        "firefly",
        "Allreduce",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

  public:
    AllreduceFuncSM( SST::Params& params ) : CollectiveTreeFuncSM( params ) { }

    virtual void handleStartEvent( SST::Event* e, Retval& retval ) {
        CollectiveTreeFuncSM::handleStartEvent( e, retval );
    }

    virtual void handleEnterEvent( Retval& retval) {
        CollectiveTreeFuncSM::handleEnterEvent( retval );
    }

    virtual std::string protocolName() { return "CtrlMsgProtocol"; }
};

}
}

#endif
