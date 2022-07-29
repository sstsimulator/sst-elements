// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NODE_OS
#define _H_VANADIS_NODE_OS

#include <unordered_set>

#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>

#include "os/callev/voscallall.h"
#include "os/vnodeoshandler.h"

#include "os/memmgr/vmemmgr.h"
#include "os/vstartthreadreq.h"
#include "os/vappruntimememory.h"

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisNodeOSComponent : public SST::Component {

public:
    SST_ELI_REGISTER_COMPONENT(VanadisNodeOSComponent, "vanadis", "VanadisNodeOS", SST_ELI_ELEMENT_VERSION(1, 0, 0),
                               "Vanadis Generic Operating System Component", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS({ "verbose", "Set the output verbosity, 0 is no output, higher is more." },
                            { "cores", "Number of cores that can request OS services via a link." },
                            { "stdout", "File path to place stdout" }, { "stderr", "File path to place stderr" },
                            { "stdin", "File path to place stdin" })

    SST_ELI_DOCUMENT_PORTS({ "core%(cores)d", "Connects to a CPU core", {} })

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS({ "mem_interface", "Interface to memory system for data access",
                                          "SST::Interface::StandardMem" })

    VanadisNodeOSComponent(SST::ComponentId_t id, SST::Params& params);
    ~VanadisNodeOSComponent();

    virtual void init(unsigned int phase);
    void handleIncomingSysCall(SST::Event* ev);

    void handleIncomingMemory(StandardMem::Request* ev) {
        auto lookup_result = ev_core_map.find(ev->getID());

        if (lookup_result == ev_core_map.end()) {
            output->fatal(CALL_INFO, -1, "Error - received a call which does not have a mapping to a core.\n");
        } else {
            output->verbose(CALL_INFO, 8, 0, "redirecting to core %" PRIu32 "...\n", lookup_result->second);
            core_handlers[lookup_result->second]->handleIncomingMemory(ev);
            ev_core_map.erase(lookup_result);
        }
    }

    void sendMemoryEvent(StandardMem::Request* ev, uint32_t core) {
        ev_core_map.insert(std::pair<StandardMem::Request::id_t, uint32_t>(ev->getID(), core));
        mem_if->send(ev);
    }

    uint64_t getSimNanoSeconds() { return getCurrentSimTimeNano(); }

private:
    VanadisNodeOSComponent();                              // for serialization only
    VanadisNodeOSComponent(const VanadisNodeOSComponent&); // do not implement
    void operator=(const VanadisNodeOSComponent&);         // do not implement

    std::function<uint64_t()> get_sim_nano;
    std::unordered_map<StandardMem::Request::id_t, uint32_t> ev_core_map;
    std::vector<SST::Link*> core_links;
    std::vector<VanadisNodeOSCoreHandler*> core_handlers;

    StandardMem* mem_if;
    VanadisMemoryManager* memory_mgr;

    VanadisELFInfo* elf_info;
    AppRuntimeMemoryMod* appRuntimeMemory;

    SST::Output* output;
};

} // namespace Vanadis
} // namespace SST

#endif
