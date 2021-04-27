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
#include <sst/core/simulation.h>

#include "arielcpu.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <time.h>

#include <string.h>

using namespace SST::ArielComponent;

ArielCPU::ArielCPU(ComponentId_t id, Params& params) :
            Component(id) {

    int verbosity = params.find<int>("verbose", 0);
    output = new SST::Output("ArielComponent[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);

    output->verbose(CALL_INFO, 1, 0, "Creating Ariel component...\n");

    core_count = (uint32_t) params.find<uint32_t>("corecount", 1);

    uint64_t max_insts = (uint64_t) params.find<uint64_t>("max_insts", 0);

    output->verbose(CALL_INFO, 1, 0, "Configuring for %" PRIu32 " cores...\n", core_count);

    uint32_t perform_checks = (uint32_t) params.find<uint32_t>("checkaddresses", 0);
    output->verbose(CALL_INFO, 1, 0, "Configuring for check addresses = %s\n", (perform_checks > 0) ? "yes" : "no");

/** This section of code preserves backward compability from the old memorymanager parameters to the new subcomponent structure. */
    // Warn about the parameters that have moved to the subcomponent
    bool found;
    std::string paramStr = params.find<std::string>("vtop", "", found);
    if (found) {
        output->verbose(CALL_INFO, 0, 0, "WARNING - ariel parameter name change: change 'vtop' to 'memmgr.vtop' in your input. The old parameter will continue to work for now but may not be supported in future releases.\n");
        // Check if both parameters are defined - if so, ignore the old one
        params.find<bool>("memmgr.vtop", 0, found);
        if (!found) params.insert("memmgr.vtop", paramStr, true);
    }

    paramStr = params.find<std::string>("pagemappolicy", "", found);
    if (found) {
        output->verbose(CALL_INFO, 0, 0, "WARNING - ariel parameter name change: change 'pagemappolicy' to 'memmgr.pagemappolicy' in your input. The old parameter will continue to work for now but may not be supported in future releases.\n");
        params.find<std::string>("memmgr.pagemappolicy", "", found);
        if (!found) params.insert("memmgr.pagemappolicy", paramStr, true);
    }

    paramStr = params.find<std::string>("translatecacheentries", "", found);
    if (found) {
        output->verbose(CALL_INFO, 0, 0, "WARNING - ariel parameter name change: change 'translatecacheentries' to 'memmgr.translatecacheentries' in your input. The old parameter will continue to work for now but may not be supported in future releases.\n");
        params.find<std::string>("memmgr.translatecacheentries", "", found);
        if (!found) params.insert("memmgr.translatecacheentries", paramStr, true);
    }
    paramStr = params.find<std::string>("memorylevels", "", found);
    if (found) {
        output->verbose(CALL_INFO, 0, 0, "WARNING - ariel parameter name change: change 'memorylevels' to 'memmgr.memorylevels' in your input. The old parameter will continue to work for now but may not be supported in future releases.\n");
        params.find<std::string>("memmgr.memorylevels", "", found);
        if (!found) params.insert("memmgr.memorylevels", paramStr, true);
    }
    paramStr = params.find<std::string>("defaultlevel", "", found);
    if (found) {
        output->verbose(CALL_INFO, 0, 0, "WARNING - ariel parameter name change: change 'defaultlevel' to 'memmgr.defaultlevel' in your input. The old parameter will continue to work for now but may not be supported in future releases.\n");
        params.find<std::string>("memmgr.defaultlevel", "", found);
        if (!found) params.insert("memmgr.defaultlevel", paramStr, true);
    }
    uint32_t memLevels = params.find<uint32_t>("memmgr.memorylevels", 1, found);
    if (!found) memLevels = params.find<int>("memorylevels", 1);
    char * level_buffer = (char*) malloc(sizeof(char) * 256);
    for (uint32_t i = 0; i < memLevels; i++) {
        // Check pagesize/pagecount/pagepopulate for each level
        sprintf(level_buffer, "pagesize%" PRIu32, i);
        paramStr = params.find<std::string>(level_buffer, "", found);
        if (found) {
            output->verbose(CALL_INFO, 0, 0, "WARNING - ariel parameter name change: change '%s' to 'memmgr.%s' in your input. The old parameter will continue to work for now but may not be supported in future releases.\n", level_buffer, level_buffer);
            sprintf(level_buffer, "memmgr.pagesize%" PRIu32, i);
            params.find<std::string>(level_buffer, "", found);
            if (!found) params.insert(level_buffer, paramStr, true);
        }
        sprintf(level_buffer, "pagecount%" PRIu32, i);
        paramStr = params.find<std::string>(level_buffer, "", found);
        if (found) {
            output->verbose(CALL_INFO, 0, 0, "WARNING - ariel parameter name change: change '%s' to 'memmgr.%s' in your input. The old parameter will continue to work for now but may not be supported in future releases.\n", level_buffer, level_buffer);
            sprintf(level_buffer, "memmgr.pagecount%" PRIu32, i);
            params.find<std::string>(level_buffer, "", found);
            if (!found) params.insert(level_buffer, paramStr, true);
        }
        sprintf(level_buffer, "page_populate_%" PRIu32, i);
        paramStr = params.find<std::string>(level_buffer, "", found);
        if (found) {
            output->verbose(CALL_INFO, 0, 0, "WARNING - ariel parameter name change: change '%s' to 'memmgr.%s' in your input. The old parameter will continue to work for now but may not be supported in future releases.\n", level_buffer, level_buffer);
            sprintf(level_buffer, "memmgr.pagepopulate%" PRIu32, i);
            params.find<std::string>(level_buffer, "", found);
            if (!found) params.insert(level_buffer, paramStr, true);
        }
    }
    free(level_buffer);
/** End memory manager subcomponent parameter translation */

    std::string memorymanager = params.find<std::string>("memmgr", "ariel.MemoryManagerSimple");
    if (NULL != (memmgr = loadUserSubComponent<ArielMemoryManager>("memmgr"))) {
        output->verbose(CALL_INFO, 1, 0, "Loaded memory manager: %s\n", memmgr->getName().c_str());
    } else {
        // Warn about memory levels and the selected memory manager if needed
        if (memorymanager == "ariel.MemoryManagerSimple" && memLevels > 1) {
            output->verbose(CALL_INFO, 1, 0, "Warning - the default 'ariel.MemoryManagerSimple' does not support multiple memory levels. Configuring anyways but memorylevels will be 1.\n");
            params.insert("memmgr.memorylevels", "1", true);
        }

        output->verbose(CALL_INFO, 1, 0, "Loading memory manager: %s\n", memorymanager.c_str());
        Params mmParams = params.get_scoped_params("memmgr");
        memmgr = loadAnonymousSubComponent<ArielMemoryManager>(memorymanager, "memmgr", 0, ComponentInfo::SHARE_STATS | ComponentInfo::INSERT_STATS, mmParams);
        if (NULL == memmgr) output->fatal(CALL_INFO, -1, "Failed to load memory manager: %s\n", memorymanager.c_str());
    }

    output->verbose(CALL_INFO, 1, 0, "Memory manager construction is completed.\n");

    uint32_t maxIssuesPerCycle   = (uint32_t) params.find<uint32_t>("maxissuepercycle", 1);
    uint32_t maxCoreQueueLen     = (uint32_t) params.find<uint32_t>("maxcorequeue", 64);
    uint32_t maxPendingTransCore = (uint32_t) params.find<uint32_t>("maxtranscore", 16);
    uint64_t cacheLineSize       = (uint64_t) params.find<uint32_t>("cachelinesize", 64);

    int gpu_e = (uint32_t) params.find<uint32_t>("gpu_enabled", 0);

#ifdef HAVE_CUDA
    if(gpu_e == 1)
        gpu_enabled = true;
    else
        gpu_enabled = false;
#endif

    /////////////////////////////////////////////////////////////////////////////////////

    frontend = loadUserSubComponent<ArielFrontend>("frontend", ComponentInfo::SHARE_NONE, core_count, maxCoreQueueLen, memmgr->getDefaultPool());
    if (!frontend) {
        // ariel.frontend.pin points to either pin2 or pin3 based on sst-elements configuration
        frontend = loadAnonymousSubComponent<ArielFrontend>("ariel.frontend.pin", "frontend", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_STATS,
                params, core_count, maxCoreQueueLen, memmgr->getDefaultPool());
    }
    if (!frontend)
        output->fatal(CALL_INFO, -1, "%s, Error: Loading frontend subcomponent failed. If Ariel was not built with Pin, user must supply a custom frontend in the input file.\n", getName().c_str());

    tunnel = frontend->getTunnel();
#ifdef HAVE_CUDA
    tunnelR = frontend->getReturnTunnel();
    tunnelD = frontend->getDataTunnel();
#endif

    /////////////////////////////////////////////////////////////////////////////////////

    std::string cpu_clock = params.find<std::string>("clock", "1GHz");
    output->verbose(CALL_INFO, 1, 0, "Registering ArielCPU clock at %s\n", cpu_clock.c_str());

    TimeConverter* timeconverter = registerClock( cpu_clock, new Clock::Handler<ArielCPU>(this, &ArielCPU::tick ) );

    output->verbose(CALL_INFO, 1, 0, "Clocks registered.\n");

    output->verbose(CALL_INFO, 1, 0, "Creating core to cache links...\n");

    output->verbose(CALL_INFO, 1, 0, "Creating processor cores and cache links...\n");

    output->verbose(CALL_INFO, 1, 0, "Configuring cores and cache links...\n");
    for(uint32_t i = 0; i < core_count; ++i) {
        cpu_cores.push_back(loadComponentExtension<ArielCore>(tunnel,
#ifdef HAVE_CUDA
                 tunnelR, tunnelD,
#endif
                 i, maxPendingTransCore, output, maxIssuesPerCycle, maxCoreQueueLen,
                 cacheLineSize, memmgr, perform_checks, params));

        // Set max number of instructions
        cpu_cores[i]->setMaxInsts(max_insts);
    }

    // Find all the components loaded into the "memory" slot
    // Make sure all cores have a loaded subcomponent in their slot
    SubComponentSlotInfo* mem = getSubComponentSlotInfo("memory");
    if (mem) {
        if (!mem->isAllPopulated())
            output->fatal(CALL_INFO, -1, "%s, Error: loading 'memory' subcomponents. All subcomponent slots from 0 to core_count must be populated. Check your input config for non-populated slots\n", getName().c_str());

        if (mem->getMaxPopulatedSlotNumber() != core_count-1)
            output->fatal(CALL_INFO, -1, "%s, Error: Loading 'memory' subcomponents and the number of subcomponents does not match the number of cores. Cores: %u, SubComps: %u. Check your input config.\n",
                    getName().c_str(), core_count, mem->getMaxPopulatedSlotNumber());

        for (int i = 0; i < core_count; i++) {
            cpu_to_cache_links.push_back(mem->create<Interfaces::SimpleMem>(i, ComponentInfo::INSERT_STATS, timeconverter, new SimpleMem::Handler<ArielCore>(cpu_cores[i], &ArielCore::handleEvent)));
            cpu_cores[i]->setCacheLink(cpu_to_cache_links[i]);
        }
    } else {
    // Load from here not the user one; let the subcomponent have our port (cache_link)
        char* link_buffer = (char*) malloc(sizeof(char) * 256);
        for (int i = 0; i < core_count; i++) {
            Params par;
            par.insert("port", "cache_link_" + std::to_string(i));
            cpu_to_cache_links.push_back(loadAnonymousSubComponent<Interfaces::SimpleMem>("memHierarchy.memInterface", "memory", i,
                        ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, par, timeconverter, new SimpleMem::Handler<ArielCore>(cpu_cores[i], &ArielCore::handleEvent)));
            cpu_cores[i]->setCacheLink(cpu_to_cache_links[i]);

#ifdef HAVE_CUDA
            if(gpu_enabled) {
               sprintf(link_buffer, "gpu_link_%" PRIu32, i);
               cpu_to_gpu_links.push_back(configureLink(link_buffer, new Event::Handler<ArielCore>(cpu_cores[i], &ArielCore::handleGpuAckEvent)));
               cpu_cores[i]->setGpuLink(cpu_to_gpu_links[i]);
               cpu_cores[i]->setGpu();
            }

            std::string executable = params.find<std::string>("executable", "");
            cpu_cores[i]->setFilePath(executable);
#endif
        }
    }


    // Register us as an important component
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    stopTicking = true;

    output->verbose(CALL_INFO, 1, 0, "Completed initialization of the Ariel CPU.\n");
    fflush(stdout);
}

void ArielCPU::init(unsigned int phase)
{
    frontend->init(phase);

    for (uint32_t i = 0; i < core_count; i++) {
        cpu_to_cache_links[i]->init(phase);
    }
}

void ArielCPU::finish() {
    for(uint32_t i = 0; i < core_count; ++i) {
        cpu_cores[i]->finishCore();
    }

    output->verbose(CALL_INFO, 1, 0, "Ariel Processor Information:\n");
    output->verbose(CALL_INFO, 1, 0, "Completed at: %" PRIu64 " nanoseconds.\n", (uint64_t) getCurrentSimTimeNano() );
    output->verbose(CALL_INFO, 1, 0, "Ariel Component Statistics (By Core)\n");
    for(uint32_t i = 0; i < core_count; ++i) {
        cpu_cores[i]->printCoreStatistics();
    }

    memmgr->printStats();
    frontend->finish();
}

bool ArielCPU::tick( SST::Cycle_t cycle) {
    stopTicking = false;
    output->verbose(CALL_INFO, 16, 0, "Main processor tick, will issue to individual cores...\n");

    tunnel->updateTime(getCurrentSimTimeNano());
    tunnel->incrementCycles();

    // Keep ticking unless one of the cores says it is time to stop.
    for(uint32_t i = 0; i < core_count; ++i) {
        cpu_cores[i]->tick();

        if(cpu_cores[i]->isCoreHalted()) {
                stopTicking = true;
                break;
        }
    }

    // Its time to end, that's all folks
    if(stopTicking) {
        primaryComponentOKToEndSim();
    }

    return stopTicking;
}

ArielCPU::~ArielCPU() { }

void ArielCPU::emergencyShutdown() {
    /* Ask the cores to finish up.  This should flush logging */
    for(uint32_t i = 0; i < core_count; ++i) {
        cpu_cores[i]->finishCore();
    }

    frontend->emergencyShutdown();
}
