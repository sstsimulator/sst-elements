// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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

EmberShmemGenerator::EmberShmemGenerator( 
            Component* owner, Params& params, std::string name) :
    EmberGenerator(owner, params, name )
{
}

EmberShmemGenerator::~EmberShmemGenerator()
{
}

void EmberShmemGenerator::completed(const SST::Output* output,
        uint64_t time )
{
}
