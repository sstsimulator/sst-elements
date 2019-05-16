// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SIEVE_MEM_MANAGER
#define _H_SIEVE_MEM_MANAGER

#include <sst/core/component.h>
#include <sst/core/output.h>

#include "sst/elements/ariel/arielmemmgr.h"

#include <stdint.h>
#include <deque>
#include <vector>
#include <unordered_map>

using namespace SST;

namespace SST {
namespace MemHierarchy {

class MemoryManagerSieve : public ArielComponent::ArielMemoryManager {

    public:
        /* SST ELI */
        SST_ELI_REGISTER_SUBCOMPONENT(MemoryManagerSieve, "memHierarchy", "MemoryManagerSieve", SST_ELI_ELEMENT_VERSION(1,0,0),
                "Memory manager for interfacing to MemSieve", "SST::ArielComponent::ArielMemoryManager")

        SST_ELI_DOCUMENT_PORTS( {"alloc_link_%(corecound)d", "Each core's link memSieve", {"memHierarchy.AllocTrackEvent"}} )

        SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( {"memmgr", "Which memory manager to use for translation", "SST::Ariel::ArielMemoryManager" } )

        /* MemoryManagerSieve */
        MemoryManagerSieve(SST::Component* owner, Params& params);
        ~MemoryManagerSieve();

        uint64_t translateAddress(uint64_t virtAddr);
        void printStats();

        void freeMalloc(const uint64_t vAddr);
        bool allocateMalloc(const uint64_t size, const uint32_t level, const uint64_t virtualAddress, const uint64_t instructionPointer, const uint32_t thread);
        
    private:
        std::vector<SST::Link*> allocLink;
        ArielMemoryManager * memmgr;
};

}
}

#endif
