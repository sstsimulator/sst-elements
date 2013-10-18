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

#include "funcSM/allreduce.h"

using namespace SST::Firefly;

AllreduceFuncSM::AllreduceFuncSM( 
            int verboseLevel, Output::output_location_t loc,
                                Info* info, ProtocolAPI* xxx ) :
    CollectiveTreeFuncSM( verboseLevel, loc, info, xxx ) 
{}

void AllreduceFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:AllreduceFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    CollectiveTreeFuncSM::handleStartEvent(e,retval);
}

void AllreduceFuncSM::handleEnterEvent( SST::Event *e, Retval& retval )
{
    CollectiveTreeFuncSM::handleEnterEvent(e,retval);
}
