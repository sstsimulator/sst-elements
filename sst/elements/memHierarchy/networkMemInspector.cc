// -*- mode: c++ -*-
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <networkMemInspector.h>
#include <memNIC.h>

namespace SST { namespace MemHierarchy {

networkMemInspector::networkMemInspector(Component *parent)
    : NetworkInspector(parent) {
    // should fix to have this be a param
    dbg.init("@R:netMemInspect::@p():@l " + parent->getName() + ": ", 0, 0, 
             Output::STDOUT);  

}

void networkMemInspector::initialize(std::string id) {
    // Init the stats
    for (int i = 0; i < LAST_CMD; ++i) {
        memCmdStat[i] = registerStatistic<uint64_t>(CommandString[i],id);
    }
}

void networkMemInspector::inspectNetworkData(SimpleNetwork::Request* req) {
    MemNIC::MemRtrEvent *mre = dynamic_cast<MemNIC::MemRtrEvent*>(req->inspectPayload());
    if (mre) {
        memCmdStat[mre->event->getCmd()]->addData(1);
    } else {
        dbg.output(CALL_INFO,"Unexpected payload encountered. Ignoring.\n");
    }
}

}} // close sst::memhierarchy namespace



