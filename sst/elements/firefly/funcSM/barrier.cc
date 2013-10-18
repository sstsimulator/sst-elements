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

#include "funcSM/barrier.h"

using namespace SST::Firefly;

BarrierFuncSM::BarrierFuncSM( 
            int verboseLevel, Output::output_location_t loc,
            Info* info, ProtocolAPI* xxx ) :
    CollectiveTreeFuncSM(verboseLevel, loc, info, xxx ) 
{
    m_dbg.setPrefix("@t:BarrierFuncSM::@p():@l ");
}

void BarrierFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:BarrierFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    BarrierStartEvent* event = static_cast<BarrierStartEvent*>(e);

    CollectiveStartEvent* tmp = new CollectiveStartEvent( NULL, NULL, 0,
                Hermes::CHAR, Hermes::MAX, 0, Hermes::GroupWorld, true );

    delete event;

    CollectiveTreeFuncSM::handleStartEvent(
                        static_cast<SST::Event*>(tmp),retval);
}

void BarrierFuncSM::handleEnterEvent( SST::Event *e, Retval& retval )
{
    CollectiveTreeFuncSM::handleEnterEvent( e, retval );
}
