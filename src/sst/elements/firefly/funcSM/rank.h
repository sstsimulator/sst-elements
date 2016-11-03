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

#ifndef COMPONENTS_FIREFLY_FUNCSM_RANK_H
#define COMPONENTS_FIREFLY_FUNCSM_RANK_H

#include "funcSM/api.h"
#include "event.h"

namespace SST {
namespace Firefly {

class RankFuncSM :  public FunctionSMInterface
{
  public:
    RankFuncSM( SST::Params& params ) : FunctionSMInterface( params ) {}

    virtual void handleStartEvent( SST::Event *e, Retval& retval ) {

        RankStartEvent* event = static_cast< RankStartEvent* >(e);
        *event->rank = m_info->getGroup(event->group)->getMyRank();
        
        m_dbg.verbose(CALL_INFO,1,0,"%d\n",*event->rank);

        retval.setExit(0);
        delete e;
    }
};

}
}

#endif
