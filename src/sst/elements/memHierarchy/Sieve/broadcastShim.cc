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

/*
 * File:   broadcastShim.cc
 */


#include <sst_config.h>

#include "broadcastShim.h"

using namespace SST;
using namespace SST::MemHierarchy;

BroadcastShim::BroadcastShim(ComponentId_t id, Params &params) : Component(id) {
    
    output_ = new SST::Output("BroadcastShim ", 1, 0, SST::Output::STDOUT);
    
    SST::Link* link;
    int linkCPU = 0;
    int linkSieve = 0;
    
    while (true) {
        std::string linkName = "cpu_alloc_link_" + std::to_string(linkCPU);
        link = configureLink(linkName, "100 ps", new Event::Handler<BroadcastShim>(this, &BroadcastShim::processCoreEvent));
        if (link) {
            cpuAllocLinks_.push_back(link);
            output_->output(CALL_INFO, "Port %d = Link %d\n", link->getId(), linkCPU);
            linkCPU++;
        } else {
            break;
        }
    }
    
    while (true) {
        std::string linkName = "sieve_alloc_link_" + std::to_string(linkSieve);
        link = configureLink(linkName, "100 ps", new Event::Handler<BroadcastShim>(this, &BroadcastShim::processSieveEvent));
        if (link) {
            sieveAllocLinks_.push_back(link);
            output_->output(CALL_INFO, "Port %d = Link %d\n", link->getId(), linkSieve);
            linkSieve++;
        } else {
            break;
        }
    }

    if (linkCPU < 1) output_->fatal(CALL_INFO, -1,"Did not find any connected links on ports cpu_alloc_link_n\n");
    if (linkSieve < 1) output_->fatal(CALL_INFO, -1,"Did not find any connected links on ports sive_alloc_link_n\n");
    
}


void BroadcastShim::processCoreEvent(SST::Event* ev) {
    ArielComponent::arielAllocTrackEvent* event = dynamic_cast<ArielComponent::arielAllocTrackEvent*>(ev);
    
    for (std::vector<Link*>::iterator it = sieveAllocLinks_.begin(); it != sieveAllocLinks_.end(); it++) {
        (*it)->send(new ArielComponent::arielAllocTrackEvent(*event));
    }
    
    delete event;
}


/* Unused in practice since alloc links are currently one way */
void BroadcastShim::processSieveEvent(SST::Event* ev) {
    delete ev;
}
