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

#ifndef COMPONENTS_FIREFLY_FUNCSM_COMMDESTROY_H
#define COMPONENTS_FIREFLY_FUNCSM_COMMDESTROY_H

#include "funcSM/barrier.h"

namespace SST {
namespace Firefly {

class CommDestroyFuncSM :  public BarrierFuncSM
{
  public:
    SST_ELI_REGISTER_MODULE(
        CommDestroyFuncSM,
        "firefly",
        "CommDestroy",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )
  public:
    CommDestroyFuncSM( SST::Params& params )
        : BarrierFuncSM( params ) {}

    virtual void handleStartEvent( SST::Event* e, Retval& retval ){
        CommDestroyStartEvent* event = static_cast<CommDestroyStartEvent*>( e );
    
        m_comm = event->comm;
        BarrierStartEvent* tmp = new BarrierStartEvent( event->comm  );
        delete event;

        BarrierFuncSM::handleStartEvent(static_cast<SST::Event*>(tmp), retval );
    }

    virtual void handleEnterEvent( Retval& retval ) {
        BarrierFuncSM::handleEnterEvent( retval );
        if ( retval.isExit() ) {
            m_info->delGroup( m_comm );
        }
    }
  private:
    MP::Communicator m_comm;
};

}
}

#endif
