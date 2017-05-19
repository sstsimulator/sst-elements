// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/component.h>

#include "emberShmemGen.h"

using namespace SST;
using namespace SST::Ember;

const char* EmberShmemGenerator::m_eventName[] = {
    FOREACH_ENUM(GENERATE_STRING)
};

EmberShmemGenerator::EmberShmemGenerator( 
            Component* owner, Params& params) :
    EmberGenerator(owner, params), 
    m_printStats( 0 )
{
    m_printStats = (uint32_t) (params.find("printStats", 0));
}

EmberShmemGenerator::~EmberShmemGenerator()
{
}

void EmberShmemGenerator::completed(const SST::Output* output,
        uint64_t time )
{
}
