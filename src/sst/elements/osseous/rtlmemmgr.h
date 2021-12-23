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


#ifndef _H_RTL_MEM_MANAGER
#define _H_RTL_MEM_MANAGER

#include <sst/core/subcomponent.h>
#include <sst/core/output.h>

#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <deque>

using namespace SST;

namespace SST {

namespace RtlComponent {

class RtlMemoryManager : public SubComponent {

    public:
        SST_ELI_REGISTER_SUBCOMPONENT_API(SST::RtlComponent::RtlMemoryManager)

        RtlMemoryManager(ComponentId_t id, Params& params) : SubComponent(id) {
            int verbosity = params.find<int>("verbose", 0);
            output = new SST::Output("RtlMemoryManager[@f:@l:@p] ",
                verbosity, 0, SST::Output::STDOUT);
        }


        ~RtlMemoryManager() {};

        /** Set default memory pool for allocation */
        virtual void setDefaultPool(uint32_t pool) {
        }

        virtual uint32_t getDefaultPool() {
            return 0;
        }

        virtual void AssignRtlMemoryManagerSimple(std::unordered_map<uint64_t, uint64_t>, std::deque<uint64_t>*, uint64_t) { }

        virtual void AssignRtlMemoryManagerCache(std::unordered_map<uint64_t, uint64_t>, uint32_t, bool) { }

        /** Return the physical address for the request virtual address */
        virtual uint64_t translateAddress(uint64_t virtAddr) = 0;

        /** Request to allocate a malloc, not supported by all memory managers */
        virtual bool allocateMalloc(const uint64_t size, const uint32_t level, const uint64_t virtualAddress, const uint64_t instructionPointer, const uint32_t thread) {
            output->verbose(CALL_INFO, 0, 0, "The instantiated RtlMemoryManager does not support malloc handling.\n");
            //fprintf(stderr, "\nAllocateMalloc is not supported\n");
            return false;
        }

        /** Request to free a malloc, not supported by all memory managers */
        virtual void freeMalloc(const uint64_t vAddr) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated RtlMemoryManager does not support malloc handling.\n");
        }

        virtual bool allocateMMAP(const uint64_t size, const uint32_t level, const uint64_t virtualAddress, const uint64_t instructionPointer, const uint32_t file, const uint32_t thread) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated RtlMemoryManager does nto support MMAP handling.\n");
            return false;
        }

        virtual void freeMMAP(const uint32_t file) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated RtlMemoryManager does not support MMAP handling.\n");
        }

        /** Print statistics: TODO move statistics to Statistics API */
        virtual void printStats() {};

    protected:
        Output* output;
};

}
}

#endif
