// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
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
#include "sst/elements/merlin/test/simple_patterns/empty.h"

#include <sst/core/interfaces/simpleNetwork.h>

namespace SST {
using namespace SST::Interfaces;

namespace Merlin {

empty_nic::empty_nic(ComponentId_t cid, Params& params) :
    Component(cid)
{
    link_control = loadUserSubComponent<SST::Interfaces::SimpleNetwork>
        ("networkIF", ComponentInfo::SHARE_NONE, 1 /* vns */);
}

empty_nic::~empty_nic()
{
    delete link_control;
}

void empty_nic::finish()
{
    link_control->finish();
}

void
empty_nic::init(unsigned int phase) {
    link_control->init(phase);
}

void
empty_nic::setup()
{
    link_control->setup();
}


} // namespace Merlin
} // namespace SST

