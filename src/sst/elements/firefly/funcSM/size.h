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

#ifndef COMPONENTS_FIREFLY_FUNCSM_SIZE_H
#define COMPONENTS_FIREFLY_FUNCSM_SIZE_H

#include "funcSM/api.h"
#include "event.h"

namespace SST {
namespace Firefly {

class SizeFuncSM :  public FunctionSMInterface
{
  public:
    SST_ELI_REGISTER_MODULE(
        SizeFuncSM,
        "firefly",
        "Size",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )
  public:
    SizeFuncSM( SST::Params& params ) : FunctionSMInterface( params ) {}

    virtual void handleStartEvent( SST::Event *e, Retval& retval ) {

        SizeStartEvent* event = static_cast< SizeStartEvent* >(e);

        m_dbg.debug(CALL_INFO,1,0,"group=%d\n",event->group);

        assert( m_info->getGroup(event->group) );
        *event->size = m_info->getGroup(event->group)->getSize();

        retval.setExit(0);
        delete event;
    }
};

}
}

#endif
