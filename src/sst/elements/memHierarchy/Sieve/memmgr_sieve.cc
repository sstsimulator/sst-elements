// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include <stdio.h>

#include <sst/core/link.h>

#include "memmgr_sieve.h"
#include "alloctrackev.h"

using namespace SST::MemHierarchy;
using namespace SST::ArielComponent;

MemoryManagerSieve::MemoryManagerSieve(ComponentId_t id, Params& params) :
            ArielMemoryManager(id, params) {

    // Load a memory manager to actually do the translation -> we're just interception allocation events

    // Attempt to load as a named subcomponent first
    if (NULL != (memmgr = loadUserSubComponent<ArielMemoryManager>("memmgr"))) {
        output->verbose(CALL_INFO, 1, 0, "Loaded memory manager: %s\n", memmgr->getName().c_str());
    } else {
        std::string memorymanager = params.find<std::string>("memmgr", "ariel.MemoryManagerSimple");
        output->verbose(CALL_INFO, 1, 0, "Loading memory manager: %s\n", memorymanager.c_str());
        Params mmParams = params.get_scoped_params("memmgr");
        memmgr = loadAnonymousSubComponent<ArielMemoryManager>(memorymanager, "memmgr", 0, ComponentInfo::SHARE_STATS | ComponentInfo::INSERT_STATS, mmParams);
        if (NULL == memmgr) output->fatal(CALL_INFO, -1, "Failed to load memory manager: %s\n", memorymanager.c_str());
    }

    // Find our links
    std::string linkprefix = "alloc_link_";
    std::string linkname = linkprefix + "0";
    int numPorts = 0;
    while (isPortConnected(linkname)) {
        SST::Link* link = configureLink(linkname, "50ps"); // No handler since never receives anything
        allocLink.push_back(link);
        numPorts++;
        linkname = linkprefix + std::to_string(numPorts);
    }

    if (numPorts == 0) {
        output->output("MemoryManagerSieve - WARNING: No allocation links found...did you mean to do this? Note: this subcomponent MUST be instantiated as a named subcomponent to work correctly!\n");
    }
}


MemoryManagerSieve::~MemoryManagerSieve() {
}


uint64_t MemoryManagerSieve::translateAddress(uint64_t virtAddr) {
    return memmgr->translateAddress(virtAddr);
}


bool MemoryManagerSieve::allocateMalloc(const uint64_t size, const uint32_t level, const uint64_t addr, const uint64_t ip, const uint32_t thread) {
    output->verbose(CALL_INFO, 4, 0, "Allocate malloc received. VA: %" PRIu64 ". Size: %" PRIu64 ".\n", addr, size);

    // Broadcast to all trackers
    for (int i = 0; i < allocLink.size(); i++) {
        allocLink[i]->send(new AllocTrackEvent(AllocTrackEvent::ALLOC, addr, size, level, ip));
    }
    return true;
}


void MemoryManagerSieve::freeMalloc(const uint64_t virtualAddress) {
    output->verbose(CALL_INFO, 4, 0, "Freeing %" PRIu64 "\n", virtualAddress);

    // Broadcast to all trackers
    for (int i = 0; i < allocLink.size(); i++) {
        allocLink[i]->send(new AllocTrackEvent(AllocTrackEvent::FREE, virtualAddress, 0, 0, 0));
    }

}

void MemoryManagerSieve::printStats() {
    memmgr->printStats();
}
