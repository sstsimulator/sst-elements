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
#include "arielmemmgr_opal.h"
#include "Opal_Event.h"

#include <sst/core/link.h>


using namespace SST::OpalComponent;

MemoryManagerOpal::MemoryManagerOpal(ComponentId_t id, Params& params) :
            ArielComponent::ArielMemoryManager(id, params) {

    // Find links
    std::string linkprefix = "opal_link_";
    std::string linkname = linkprefix + "0";
    int numPorts = 0;

    std::string latency = params.find<std::string>("opal_latency", "32ps");

    while (isPortConnected(linkname)) {
        SST::Link* link = configureLink(linkname, latency, new Event::Handler<MemoryManagerOpal>(this, &MemoryManagerOpal::handleInterrupt));
        opalLink.push_back(link);
        numPorts++;
        linkname = linkprefix + std::to_string(numPorts);
    }

    std::string translatorstr = params.find<std::string>("translator", "ariel.MemoryManagerSimple");
    if (NULL != (temp_translator = loadUserSubComponent<ArielMemoryManager>("translator"))) {
        output->verbose(CALL_INFO, 1, 0, "Opal is using named subcomponent translator\n");
    } else {
        int memLevels = params.find<int>("memmgr.memorylevels", 1);
        if (translatorstr == "ariel.MemoryManagerSimple" && memLevels > 1) {
            output->verbose(CALL_INFO, 1, 0, "Warning - the default 'ariel.MemoryManagerSimple' does not support multiple memory levels. Configuring anyways but memorylevels will be 1.\n");
            params.insert("memmgr.memorylevels", "1", true);
        }
        output->verbose(CALL_INFO, 1, 0, "Loading memory manager: %s\n", translatorstr.c_str());
        Params translatorParams = params.get_scoped_params("memmgr");
        temp_translator = loadAnonymousSubComponent<ArielMemoryManager>(translatorstr, "translator", 0, ComponentInfo::SHARE_STATS | ComponentInfo::INSERT_STATS, translatorParams);
        if (NULL == temp_translator)
            output->fatal(CALL_INFO, -1, "Failed to load memory manager: %s\n", translatorstr.c_str());
    }
}


MemoryManagerOpal::~MemoryManagerOpal() {
}


void MemoryManagerOpal::handleInterrupt(SST::Event *event) {
    OpalEvent * ev = dynamic_cast<OpalComponent::OpalEvent*>(event); // TODO can we static_cast instead?
    output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " handling opal interrupt event\n", ev->getCoreId());

    switch(ev->getType()) {
        case SST::OpalComponent::EventType::SHOOTDOWN:
            (*(interruptHandler[ev->getCoreId()]))(ArielComponent::ArielMemoryManager::InterruptAction::STALL);
            break;
        case SST::OpalComponent::EventType::SDACK:
            (*(interruptHandler[ev->getCoreId()]))(ArielComponent::ArielMemoryManager::InterruptAction::UNSTALL);
            break;
        default:
            output->fatal(CALL_INFO, -4, "Opal event interrupt to core: %" PRIu32 " was not valid.\n", ev->getCoreId());
    }
}

bool MemoryManagerOpal::allocateMalloc(const uint64_t size, const uint32_t level, const uint64_t addr, const uint64_t ip, const uint32_t thread) {
    OpalEvent * tse = new OpalEvent(OpalComponent::EventType::HINT, level, addr, size, thread);
    opalLink[thread]->send(tse);

    return temp_translator->allocateMalloc(size, level, addr, ip, thread);
}

bool MemoryManagerOpal::allocateMMAP(const uint64_t size, const uint32_t level, const uint64_t addr, const uint64_t ip, const uint32_t file, const uint32_t thread) {
    OpalEvent * tse = new OpalEvent(OpalComponent::EventType::HINT, level, addr, size, thread);
    tse->setFileId(file);
    output->output("Before sending to Opal.. file ID is: %" PRIu32 "\n", file);
    opalLink[thread]->send(tse);
    return true;
}

void MemoryManagerOpal::freeMalloc(const uint64_t virtualAddress) {
    temp_translator->freeMalloc(virtualAddress);
}

void MemoryManagerOpal::freeMMAP(const uint32_t file) {
}


void MemoryManagerOpal::setDefaultPool(uint32_t pool) {
    temp_translator->setDefaultPool(pool);
}

uint32_t MemoryManagerOpal::getDefaultPool() {
    return temp_translator->getDefaultPool();
}

uint64_t MemoryManagerOpal::translateAddress(uint64_t virtAddr) {
    return temp_translator->translateAddress(virtAddr);
}

void MemoryManagerOpal::printStats() {
    temp_translator->printStats();
}

