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


#ifndef _H_OPAL_PAGE_FAULT_HANDLER_OPAL
#define _H_OPAL_PAGE_FAULT_HANDLER_OPAL

#include <sst/core/component.h>
#include <sst/core/output.h>

#include "sst/elements/Samba/PageFaultHandler.h"

#include <stdint.h>
#include <deque>
#include <vector>
#include <unordered_map>

using namespace SST;

namespace SST {
namespace OpalComponent {

class PageFaultHandler : public SambaComponent::PageFaultHandler {

    public:
        /* SST ELI */
        SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(PageFaultHandler, "Opal", "PageFaultHandler", SST_ELI_ELEMENT_VERSION(1,0,0),
                "Page fault hander uses the Opal memory allocation component", SST::SambaComponent::PageFaultHandler)

        SST_ELI_DOCUMENT_PARAMS(
                { "opal_latency",   "latency to communicate to the Opal manager", "32ps"} )

        SST_ELI_DOCUMENT_PORTS( {"opal_link_%(corecound)d", "Each core's mmu link to the Opal page fault handler", {"Opal.OpalEvent"}} )

        /* MemoryManagerOpal */
        PageFaultHandler(ComponentId_t id, Params& params);
        ~PageFaultHandler();

        void handleEvent(SST::Event * event);
        void allocatePage(const uint32_t thread, const uint32_t level, const uint64_t virtualAddress, const uint64_t size);

    private:

        std::vector<SST::Link*> opalLink;
};

}
}

#endif
