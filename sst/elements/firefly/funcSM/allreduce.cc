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
            Info* info, SST::Link*& progressLink, 
            ProtocolAPI* xxx, IO::Interface* io ) :
    CollectiveTreeFuncSM( verboseLevel,loc,info,progressLink,xxx,io  ) 
{}

void AllreduceFuncSM::handleEnterEvent( SST::Event *e) 
{
    CollectiveTreeFuncSM::handleEnterEvent(e);
}

void AllreduceFuncSM::handleProgressEvent( SST::Event *e )
{
    CollectiveTreeFuncSM::handleProgressEvent(e);
}
