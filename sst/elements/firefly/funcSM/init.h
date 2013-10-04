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

#ifndef COMPONENTS_FIREFLY_FUNCSM_INIT_H
#define COMPONENTS_FIREFLY_FUNCSM_INIT_H

#include "info.h"
#include "funcSM/api.h"

namespace SST {
namespace Firefly {

class InitFuncSM :  public FunctionSMInterface 
{
  public:
    InitFuncSM( int verboseLevel, Output::output_location_t loc,
            Info* info ) :
        FunctionSMInterface( verboseLevel, loc, info ) 
    {
        m_dbg.setPrefix("@t:InitFuncSM::@p():@l ");
    }

    virtual void handleStartEvent( SST::Event *e ) {
        if ( m_setPrefix ) {
            char buffer[100];
            snprintf(buffer,100,"@t:%d:%d:InitFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
            m_dbg.setPrefix(buffer);

            m_setPrefix = false;
        }

        m_dbg.verbose(CALL_INFO,1,0,"\n");
        exit( static_cast< SMStartEvent* >(e), 0 );
        delete e;
    }

    virtual const char* name() {
       return "Init"; 
    }

  private:
};

}
}

#endif
