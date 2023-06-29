// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <queue>
#include <sst_config.h>
#include "agile-io-consumer.h"
#include "emberevent.h"
#include "mpi/embermpigen.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <climits>

using namespace SST;
using namespace SST::Ember;



agileIOconsumer::agileIOconsumer(SST::ComponentId_t id, Params& prms) : EmberMessagePassingGenerator(id, prms, "Null")
{
}

bool
agileIOconsumer::generate(std::queue<EmberEvent*>& evQ)
{
}

void
agileIOconsumer::read_request()
{
}

int
agileIOconsumer::write_data()
{
    return 0;
}

int
agileIOconsumer::num_io_nodes()
{
    return 0;
}
