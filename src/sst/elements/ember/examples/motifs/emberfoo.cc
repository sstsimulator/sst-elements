// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "emberfoo.h"

using namespace SST::Ember;

Foo::Foo(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "Null" )
{
}

bool Foo::generate( std::queue<EmberEvent*>& evQ)
{
    return true;
}
