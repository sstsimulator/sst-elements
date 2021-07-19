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


#ifndef _H_OPAL_MEM_MANAGER_OPAL
#define _H_OPAL_MEM_MANAGER_OPAL

#include <sst/core/component.h>
#include <sst/core/output.h>

#include "sst/elements/ariel/arielmemmgr.h"

#include <stdint.h>
#include <deque>
#include <vector>
#include <unordered_map>

using namespace SST;

namespace SST {
namespace OpalComponent {

class MemoryManagerOpal : public ArielComponent::ArielMemoryManager {

    public:
        /* SST ELI */
        SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MemoryManagerOpal, "Opal", "MemoryManagerOpal", SST_ELI_ELEMENT_VERSION(1,0,0),
                "Memory manager which uses the Opal memory allocation component", SST::ArielComponent::ArielMemoryManager)

        SST_ELI_DOCUMENT_PARAMS(
        		{ "corecount", "Sets the verbosity of the memory manager output", "1"},
                { "opal_latency",   "latency to communicate to the Opal manager", "32ps"},
                { "translator",     "(temporary) translation memory manager to actually translate addresses for now", "MemoryManagerSimple"} )

        SST_ELI_DOCUMENT_PORTS( {"opal_link_%(corecound)d", "Each core's link to the Opal memory manager", {"Opal.OpalEvent"}} )

        SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( { "translator", "Temporarily, which memory manager to use to translate addresses", "SST::Ariel::ArielMemoryManager" } )

        /* MemoryManagerOpal */
        MemoryManagerOpal(ComponentId_t id, Params& params);
        ~MemoryManagerOpal();

        /* Call through to temporary translator */
        void setDefaultPool(uint32_t pool);
        uint32_t getDefaultPool();

        uint64_t translateAddress(uint64_t virtAddr);
        void printStats();

        /* Call through to Opal */
        bool allocateMalloc(const uint64_t size, const uint32_t level, const uint64_t virtualAddress, const uint64_t instructionPointer, const uint32_t thread);
        bool allocateMMAP(const uint64_t size, const uint32_t level, const uint64_t virtualAddress, const uint64_t instructionPointer, const uint32_t file, const uint32_t thread);
        void freeMalloc(const uint64_t vAddr);
        void freeMMAP(const uint32_t file);

        void handleInterrupt(SST::Event * event);

    private:
        ArielMemoryManager* temp_translator;    // Temporary while Opal still uses Ariel's built-in translator

        std::vector<SST::Link*> opalLink;
};

}
}

#endif
