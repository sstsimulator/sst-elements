// -*- mode: c++ -*-
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
#include <networkMemInspector.h>
#include <memNIC.h>

namespace SST { namespace MemHierarchy {

networkMemInspector::networkMemInspector(Component *parent, Params &params)
    : NetworkInspector(parent) {
    // should fix to have this be a param
    dbg.init("@R:netMemInspect::@p():@l " + parent->getName() + ": ", 0, 0, 
             Output::STDOUT);  

}

void networkMemInspector::initialize(std::string id) {
    // Init the stats
    for (int i = 0; i < (int)Command::LAST_CMD; ++i) {
        memCmdStat[i] = registerStatistic<uint64_t>(CommandString[i],id);
    }
}

void networkMemInspector::inspectNetworkData(SimpleNetwork::Request* req) {
    MemNIC::MemRtrEvent *mre = dynamic_cast<MemNIC::MemRtrEvent*>(req->inspectPayload());
    if (mre) {
        memCmdStat[(int)mre->event->getCmd()]->addData(1);
    } else {
        dbg.output(CALL_INFO,"Unexpected payload encountered. Ignoring.\n");
    }
}

}} // close sst::memhierarchy namespace



