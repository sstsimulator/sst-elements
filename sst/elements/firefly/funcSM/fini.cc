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


#include <sst_config.h>
#include "sst/core/serialization.h"

#include "funcSM/fini.h"

using namespace SST::Firefly;

FiniFuncSM::FiniFuncSM( int verboseLevel, Output::output_location_t loc,
                    Info* info, SST::Link*& progressLink, ProtocolAPI* xxx ) :
    BarrierFuncSM(verboseLevel,loc,info,progressLink,xxx) 
{
    m_dbg.setPrefix("@t:FiniFuncSM::@p():@l ");
}

void FiniFuncSM::handleEnterEvent( SST::Event *e) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:FiniFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    FiniEnterEvent* event = static_cast<FiniEnterEvent*>(e);
    BarrierEnterEvent* tmp = new BarrierEnterEvent( 0,
                            event->retFunc, Hermes::GroupWorld  );

    tmp->retLink = event->retLink;

    delete event;

    BarrierFuncSM::handleEnterEvent(static_cast<SST::Event*>(tmp));
}

void FiniFuncSM::handleProgressEvent( SST::Event *e )
{
    BarrierFuncSM::handleProgressEvent(e);
}
