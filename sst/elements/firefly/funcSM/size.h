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

#ifndef COMPONENTS_FIREFLY_FUNCSM_SIZE_H
#define COMPONENTS_FIREFLY_FUNCSM_SIZE_H

#include "funcSM/api.h"

namespace SST {
namespace Firefly {

class SizeFuncSM :  public FunctionSMInterface
{
  public:
    SizeFuncSM( int verboseLevel, Output::output_location_t loc, Info* info ) :
        FunctionSMInterface(verboseLevel,loc,info)
    { 
        m_dbg.setPrefix("@t:SizeFuncSM::@p():@l ");
    }

    virtual void handleStartEvent( SST::Event *e) {
        if ( m_setPrefix ) {
            char buffer[100];
            snprintf(buffer,100,"@t:%d:%d:SizeFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
            m_dbg.setPrefix(buffer);

            m_setPrefix = false;
        }

        m_event = static_cast< SizeStartEvent* >(e);

        m_dbg.verbose(CALL_INFO,1,0,"\n");

        *m_event->size = m_info->getGroup(m_event->group)->size();
        exit( static_cast<SMStartEvent*>(e), 0 );
        delete m_event;
    }

    virtual const char* name() {
       return "Size"; 
    }

  private:
    SizeStartEvent* m_event;
};

}
}

#endif
