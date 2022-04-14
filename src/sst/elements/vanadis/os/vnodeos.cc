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
#include <sst/core/component.h>

#include <functional>

#include "os/resp/voscallresp.h"
#include "os/resp/vosexitresp.h"
#include "os/vnodeos.h"
#include "os/voscallev.h"
#include "os/velfloader.h"
#include "os/vstartthreadreq.h"

using namespace SST::Vanadis;

VanadisNodeOSComponent::VanadisNodeOSComponent(SST::ComponentId_t id, SST::Params& params) : SST::Component(id), startThreadReq(NULL) {

    const uint32_t verbosity = params.find<uint32_t>("verbose", 0);
    output = new SST::Output("[node-os]: ", verbosity, 0, SST::Output::STDOUT);

    const uint32_t core_count = params.find<uint32_t>("cores", 0);

    std::string binary_img = params.find<std::string>("executable", "");

    if ( "" == binary_img ) {
        output->fatal(CALL_INFO, -1, "No executable specified, will not perform any binary load.\n");
        elf_info = nullptr;
    } else {
        output->verbose(CALL_INFO, 2, 0, "Executable: %s\n", binary_img.c_str());
        elf_info = readBinaryELFInfo(output, binary_img.c_str());
        elf_info->print(output);

        if ( elf_info->isDynamicExecutable() ) {
            output->fatal(
                CALL_INFO, -1,
                "--> error - executable is dynamically linked. Only static "
                "executables are currently supported for simulation.\n");
        }
        else {
            output->verbose(CALL_INFO, 2, 0, "--> executable is identified as static linked\n");
        }
    }
    elf_info->print(output);

    Params tmpParams = params.get_scoped_params("app");
    std::string modName = params.find<std::string>("appRuntimeMemoryMod","");
    appRuntimeMemory = loadModule<AppRuntimeMemoryMod>(modName, tmpParams );

    output->verbose(CALL_INFO, 1, 0, "Configuring the memory interface...\n");
    mem_if = loadUserSubComponent<Interfaces::StandardMem>("mem_interface", ComponentInfo::SHARE_NONE,
                                                         getTimeConverter("1ps"),
                                                         new StandardMem::Handler<SST::Vanadis::VanadisNodeOSComponent>(
                                                             this, &VanadisNodeOSComponent::handleIncomingMemory));

    output->verbose(CALL_INFO, 1, 0, "Configuring for %" PRIu32 " core links...\n", core_count);
    core_links.reserve(core_count);

    char* port_name_buffer = new char[128];

    std::string stdout_path = params.find<std::string>("stdout", "stdout-" + getName());
    std::string stderr_path = params.find<std::string>("stderr", "stderr-" + getName());
    std::string stdin_path = params.find<std::string>("stdin", "");

    get_sim_nano = std::bind(&VanadisNodeOSComponent::getSimNanoSeconds, this);

    uint64_t heap_start = params.find<uint64_t>("heap_start", 0);
    uint64_t heap_end = params.find<uint64_t>("heap_end", 0);
    uint64_t heap_page_size = params.find<uint64_t>("page_size", 4096);
    int heap_verbose = params.find<int>("heap_verbose", 0);

    output->verbose(CALL_INFO, 1, 0, "-> configuring mmap page range start: 0x%llx\n", heap_start);
    output->verbose(CALL_INFO, 1, 0, "-> configuring mmap page range end:   0x%llx\n", heap_end);
    output->verbose(CALL_INFO, 1, 0, "-> implies:                           %" PRIu64 " pages\n",
                    (heap_end - heap_start) / heap_page_size);
    output->verbose(CALL_INFO, 1, 0, "-> configuring mmap page size:        %" PRIu64 " bytes\n", heap_page_size);

    memory_mgr = new VanadisMemoryManager(heap_verbose, heap_start, heap_end, heap_page_size);

    for (uint32_t i = 0; i < core_count; ++i) {
        snprintf(port_name_buffer, 128, "core%" PRIu32 "", i);
        output->verbose(CALL_INFO, 1, 0, "---> processing link %s...\n", port_name_buffer);

        SST::Link* core_link = configureLink(
            port_name_buffer, "0ns",
            new Event::Handler<VanadisNodeOSComponent>(this, &VanadisNodeOSComponent::handleIncomingSysCall));

        if (nullptr == core_link) {
            output->fatal(CALL_INFO, -1, "Error: unable to configure link: %s\n", port_name_buffer);
        } else {
            output->verbose(CALL_INFO, 8, 0, "configuring link %s...\n", port_name_buffer);
            core_links.push_back(core_link);
        }

        VanadisNodeOSCoreHandler* new_core_handler = new VanadisNodeOSCoreHandler(
            verbosity, i, (stdin_path == "") ? nullptr : stdin_path.c_str(), stdout_path.c_str(), stderr_path.c_str());
        new_core_handler->setLink(core_link);
        new_core_handler->setSimTimeNano(get_sim_nano);
        new_core_handler->setMemoryManager(memory_mgr);

        std::function<void(StandardMem::Request*, uint32_t)> core_callback
            = std::bind(&VanadisNodeOSComponent::sendMemoryEvent, this, std::placeholders::_1, std::placeholders::_2);
        new_core_handler->setSendMemoryCallback(core_callback);

        core_handlers.push_back(new_core_handler);
    }

    delete[] port_name_buffer;
}

VanadisNodeOSComponent::~VanadisNodeOSComponent() {
    for (VanadisNodeOSCoreHandler* next_core_handler : core_handlers) {
        delete next_core_handler;
    }

    delete output;
    delete memory_mgr;
}

void
VanadisNodeOSComponent::init(unsigned int phase) {
    output->verbose(CALL_INFO, 1, 0, "Performing init-phase %u...\n", phase);
    mem_if->init(phase);

    // Trigger each core handler for initialization phase
    for (VanadisNodeOSCoreHandler* next_handler : core_handlers) {
        next_handler->init(phase);
    }

    if ( 0 == phase ) {

        uint64_t stack_start = appRuntimeMemory->configure(output,mem_if,elf_info);
        uint64_t brk = Vanadis::loadElfFile( output, mem_if, elf_info );
        // we are starting the thread 0 on core 0 
        core_handlers[0]->setBrk( brk );
        uint64_t entry = elf_info->getEntryPoint();
        output->verbose(CALL_INFO, 1, 0, "stack_start=%#" PRIx64 " entry=%#" PRIx64 " brk=%#" PRIx64 "\n",stack_start, entry, brk );
        startThreadReq = new VanadisStartThreadReq( 0, stack_start, entry );
    }
}

void VanadisNodeOSComponent::setup() {
    if ( startThreadReq ) {
        core_links[0]->send(startThreadReq);
    }
}

void
VanadisNodeOSComponent::handleIncomingSysCall(SST::Event* ev) {
    VanadisSyscallEvent* sys_ev = dynamic_cast<VanadisSyscallEvent*>(ev);

    if (nullptr == sys_ev) {
        output->fatal(CALL_INFO, -1,
                      "Error - received an event in the OS, but cannot cast it to "
                      "a system-call event.\n");
    }

    output->verbose(CALL_INFO, 16, 0, "Call from core: %" PRIu32 ", thr: %" PRIu32 " -> calling handler...\n",
                    sys_ev->getCoreID(), sys_ev->getThreadID());

    core_handlers[sys_ev->getCoreID()]->handleIncomingSyscall(sys_ev);
}
