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


#ifndef _H_ARIEL_MEM_MANAGER
#define _H_ARIEL_MEM_MANAGER

#include <sst/core/subcomponent.h>
#include <sst/core/output.h>

#include <stdint.h>
#include <vector>

using namespace SST;

namespace SST {

namespace ArielComponent {

class ArielMemoryManager : public SubComponent {

    public:
        SST_ELI_REGISTER_SUBCOMPONENT_API(SST::ArielComponent::ArielMemoryManager)

        enum class InterruptAction { STALL, UNSTALL };

        ArielMemoryManager(ComponentId_t id, Params& params) : SubComponent(id) {
            int verbosity = params.find<int>("verbose", 0);
            output = new SST::Output("ArielMemoryManager[@f:@l:@p] ",
                verbosity, 0, SST::Output::STDOUT);
        }

        ArielMemoryManager(Component* comp, Params& params) : SubComponent(comp) {
            output = new SST::Output("", 0, 0, SST::Output::STDOUT);
            output->fatal(CALL_INFO, -1, "Error: ArielMemoryManager subcomponents do not support loading using legacy load functions");
        }

        ~ArielMemoryManager() {};

        /** Set default memory pool for allocation */
        virtual void setDefaultPool(uint32_t pool) {
        }

        virtual uint32_t getDefaultPool() {
            return 0;
        }

        /** Return the physical address for the request virtual address */
        virtual uint64_t translateAddress(uint64_t virtAddr) = 0;

        /** Request to allocate a malloc, not supported by all memory managers */
        virtual bool allocateMalloc(const uint64_t size, const uint32_t level, const uint64_t virtualAddress, const uint64_t instructionPointer, const uint32_t thread) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated ArielMemoryManager does not support malloc handling.\n");
            return false;
        }

        /** Request to free a malloc, not supported by all memory managers */
        virtual void freeMalloc(const uint64_t vAddr) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated ArielMemoryManager does not support malloc handling.\n");
        }

        virtual bool allocateMMAP(const uint64_t size, const uint32_t level, const uint64_t virtualAddress, const uint64_t instructionPointer, const uint32_t file, const uint32_t thread) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated ArielMemoryManager does nto support MMAP handling.\n");
            return false;
        }
        
        virtual void freeMMAP(const uint32_t file) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated ArielMemoryManager does not support MMAP handling.\n");
        }

        /** Print statistics: TODO move statistics to Statistics API */
        virtual void printStats() {};

        class InterruptHandlerBase {
            public:
                virtual bool operator()(InterruptAction) = 0;
                virtual ~InterruptHandlerBase() {}
        };

        template <typename classT>
            class InterruptHandler : public InterruptHandlerBase {
                private:
                    typedef bool (classT::*PtrMember)(InterruptAction);
                    classT* object;
                    const PtrMember member;
                public:
                    InterruptHandler( classT* const obj, PtrMember mem ) :
                        object(obj),
                        member(mem)
                    {}

                    bool operator()(InterruptAction action) {
                        return (object->*member)(action);
                    }
        };

        /* Register handler */
        virtual void registerInterruptHandler(uint32_t core, InterruptHandlerBase *handler) {
            if (interruptHandler.size() <= core) {
                interruptHandler.resize(core + 1);
            }
            interruptHandler[core] = handler;
        }

    protected:
        Output* output;

        std::vector<InterruptHandlerBase*> interruptHandler;
};

}
}

#endif
