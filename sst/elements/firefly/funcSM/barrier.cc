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
#include "sst/core/serialization/element.h"

#include "funcSM/barrier.h"

using namespace SST::Firefly;

BarrierFuncSM::BarrierFuncSM( 
            int verboseLevel, Output::output_location_t loc,
            Info* info, SST::Link*& progressLink, 
            ProtocolAPI* xxx, IO::Interface*  io ) :
    CollectiveTreeFuncSM(verboseLevel,loc,info,progressLink, xxx, io ) 
{
    m_dbg.setPrefix("@t:BarrierFuncSM::@p():@l ");
}

void BarrierFuncSM::handleEnterEvent( SST::Event *e) 
{
    BarrierEnterEvent* event = static_cast<BarrierEnterEvent*>(e);

    CollectiveEnterEvent* tmp = new CollectiveEnterEvent( 0,
                event->retFunc, NULL, NULL, 0,
                Hermes::CHAR, Hermes::MAX, 0, Hermes::GroupWorld, true );

    delete event;

    tmp->retLink = event->retLink;

    CollectiveTreeFuncSM::handleEnterEvent(static_cast<SST::Event*>(tmp));
}

void BarrierFuncSM::handleProgressEvent( SST::Event *e )
{
    CollectiveTreeFuncSM::handleProgressEvent(e);
}
