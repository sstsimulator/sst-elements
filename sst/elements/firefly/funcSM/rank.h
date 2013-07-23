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

#ifndef COMPONENTS_FIREFLY_FUNCSM_RANK_H
#define COMPONENTS_FIREFLY_FUNCSM_RANK_H

#include <sst/core/output.h>

#include "funcSM/api.h"

namespace SST {
namespace Firefly {

class RankFuncSM :  public FunctionSMInterface
{
  public:
    RankFuncSM( int verboseLevel, Output::output_location_t loc,
            Info* info ) :
        FunctionSMInterface(verboseLevel,loc,info)
    {
        m_dbg.setPrefix("@t:RankFuncSM::@p():@l ");
    }

    virtual void handleEnterEvent( SST::Event *e) {

        RankEnterEvent* event = static_cast< RankEnterEvent* >(e);
        *event->rank = m_info->getGroup(event->group)->getMyRank();
        
        m_dbg.verbose(CALL_INFO,1,0,"%d\n",*event->rank);
        exit( static_cast<SMEnterEvent*>(e), 0 );
        delete e;
    }

    virtual const char* name() {
       return "Rank"; 
    }
};

}
}

#endif
