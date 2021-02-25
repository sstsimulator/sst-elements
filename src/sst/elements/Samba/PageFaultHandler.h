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

/* Author: Vamsee Reddy Kommareddy
 * E-mail: vamseereddy8@knights.ucf.edu
 */


#ifndef _H_SAMBA_PAGE_FAULT_HANDLER
#define _H_SAMBA_PAGE_FAULT_HANDLER

#include <sst/core/subcomponent.h>
#include <sst/core/output.h>

#include <stdint.h>
#include <vector>

using namespace SST;

namespace SST {

namespace SambaComponent {

class PageFaultHandler : public SubComponent {

    public:
        SST_ELI_REGISTER_SUBCOMPONENT_API(SST::SambaComponent::PageFaultHandler)

        enum class PageFaultHandlerAction { RESPONSE, INVALIDADDR, SHOOTDOWN };
        typedef struct {
        	PageFaultHandlerAction action;
        	uint64_t pAddress;
        	uint64_t vAddress;
        	uint64_t size;
        }PageFaultHandlerPacket;

        PageFaultHandler(ComponentId_t id, Params& params) : SubComponent(id) {
            int verbosity = params.find<int>("verbose", 0);
            output = new SST::Output("PageFaultHandler[@f:@l:@p] ",
                verbosity, 0, SST::Output::STDOUT);
        }
        ~PageFaultHandler() {};

        /** Request to allocate a page */
        virtual void allocatePage(const uint32_t thread, const uint32_t level, const uint64_t virtualAddress, const uint64_t size) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated PageFaultHandler does not support explicit page fault handling.\n");
        }

        class PageFaultHandlerInterfaceBase {
            public:
        		virtual bool operator()(PageFaultHandlerPacket) = 0;
                virtual ~PageFaultHandlerInterfaceBase() {}
        };

        template <typename classT>
            class PageFaultHandlerInterface : public PageFaultHandlerInterfaceBase {
                private:
                    typedef bool (classT::*PtrMember)(PageFaultHandlerPacket);
                    classT* object;
                    const PtrMember member;
                public:
                    PageFaultHandlerInterface( classT* const obj, PtrMember mem ) :
                        object(obj),
                        member(mem)
                    {}

                    bool operator()(PageFaultHandlerPacket pkt) {
                        return (object->*member)(pkt);
                    }
        };

        /* Register handler */
        void registerPageFaultHandler(uint32_t core, PageFaultHandlerInterfaceBase *handler) {
        	std::cout << getName().c_str() << " register handler" << std::endl;
        	if (pageFaultHandlerInterface.size() <= core) {
            	pageFaultHandlerInterface.resize(core + 1);
            }
            pageFaultHandlerInterface[core] = handler;

        }

    protected:
        Output* output;

        std::vector<PageFaultHandlerInterfaceBase*> pageFaultHandlerInterface;
};

}
}

#endif
