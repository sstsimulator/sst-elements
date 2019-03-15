// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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

#include <signal.h>
#if !defined(SST_COMPILE_MACOSX)
#include <sys/prctl.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <time.h>

#include <string.h>

#define ARIEL_INNER_STRINGIZE(input) #input
#define ARIEL_STRINGIZE(input) ARIEL_INNER_STRINGIZE(input)

using namespace SST::ArielComponent;

ArielCPU::ArielCPU(ComponentId_t id, Params& params) :
            Component(id) {

    int verbosity = params.find<int>("verbose", 0);
    output = new SST::Output("ArielComponent[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);

    // see if we should send allocation events out on links
    useAllocTracker = params.find<int>("alloctracker", 0);

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
    if (!memorymanager.empty()) {
        // Warn about memory levels and the selected memory manager if needed
        if (memorymanager == "ariel.MemoryManagerSimple" && memLevels > 1) {
            output->verbose(CALL_INFO, 1, 0, "Warning - the default 'ariel.MemoryManagerSimple' does not support multiple memory levels. Configuring anyways but memorylevels will be 1.\n");
            params.insert("memmgr.memorylevels", "1", true);
        } else if (memorymanager == "ariel.MemoryManagerMalloc" && memLevels == 1) {
            output->verbose(CALL_INFO, 1, 0, "Warning - 'ariel.MemoryManagerMalloc' should only be used if memLevels > 1. Reverting to 'ariel.MemoryManagerSimple'\n");
            memorymanager = "ariel.MemoryManagerSimple";
        }

        output->verbose(CALL_INFO, 1, 0, "Loading memory manger: %s\n", memorymanager.c_str());
        Params mmParams = params.find_prefix_params("memmgr.");
        memmgr = dynamic_cast<ArielMemoryManager*>( loadSubComponent(memorymanager, this, mmParams));
        if (NULL == memmgr) output->fatal(CALL_INFO, -1, "Failed to load memory manager: %s\n", memorymanager.c_str());
    } else {
        output->fatal(CALL_INFO, -1, "Failed to load memory manager: no manager specified. Please set the 'memmgr' parameter in your input deck\n");
    }

    output->verbose(CALL_INFO, 1, 0, "Memory manager construction is completed.\n");

    uint32_t maxIssuesPerCycle   = (uint32_t) params.find<uint32_t>("maxissuepercycle", 1);
    uint32_t maxCoreQueueLen     = (uint32_t) params.find<uint32_t>("maxcorequeue", 64);
    uint32_t maxPendingTransCore = (uint32_t) params.find<uint32_t>("maxtranscore", 16);
    uint64_t cacheLineSize       = (uint64_t) params.find<uint32_t>("cachelinesize", 64);
    int op_e = (uint32_t) params.find<uint32_t>("opal_enabled", 0);

    if(op_e == 1)
        opal_enabled = true;
    else
        opal_enabled = false;

    /////////////////////////////////////////////////////////////////////////////////////


    char* tool_path = (char*) malloc(sizeof(char) * 1024);

#ifdef SST_COMPILE_MACOSX
    sprintf(tool_path, "%s/fesimple.dylib", ARIEL_STRINGIZE(ARIEL_TOOL_DIR));
#else
    sprintf(tool_path, "%s/fesimple.so", ARIEL_STRINGIZE(ARIEL_TOOL_DIR));
#endif

    std::string ariel_tool = params.find<std::string>("arieltool", tool_path);
    if("" == ariel_tool) {
        output->fatal(CALL_INFO, -1, "The arieltool parameter specifying which PIN tool to run was not specified\n");
    }

    free(tool_path);

    std::string executable = params.find<std::string>("executable", "");
    if("" == executable) {
        output->fatal(CALL_INFO, -1, "The input deck did not specify an executable to be run against PIN\n");
    }

    uint32_t app_argc = (uint32_t) params.find<uint32_t>("appargcount", 0);
    output->verbose(CALL_INFO, 1, 0, "Model specifies that there are %" PRIu32 " application arguments\n", app_argc);

    uint32_t pin_startup_mode = (uint32_t) params.find<uint32_t>("arielmode", 2);
    uint32_t intercept_mem_allocations = (uint32_t) params.find<uint32_t>("arielinterceptcalls", 0);

    // Always enable allocation interception if using opal...
    if (opal_enabled)
        intercept_mem_allocations = 1;

    switch(intercept_mem_allocations) {
    case 0:
        output->verbose(CALL_INFO, 1, 0, "Interception and instrumentation of multi-level memory and malloc/free calls is DISABLED.\n");
        break;
    default:
        output->verbose(CALL_INFO, 1, 0, "Interception and instrumentation of multi-level memory and malloc/free calls is ENABLED.\n");
        break;
    }

    uint32_t keep_malloc_stack_trace = (uint32_t) params.find<uint32_t>("arielstack", 0);
    output->verbose(CALL_INFO, 1, 0, "Tracking the stack and dumping on malloc calls is %s.\n",
            keep_malloc_stack_trace == 1 ? "ENABLED" : "DISABLED");

    std::string malloc_map_filename = params.find<std::string>("mallocmapfile", "");
    if (malloc_map_filename == "") {
        output->verbose(CALL_INFO, 1, 0, "Malloc map file is DISABLED\n");
    } else {
        output->verbose(CALL_INFO, 1, 0, "Malloc map file is ENABLED, using file '%s'\n", malloc_map_filename.c_str());
    }

    tunnel = new ArielTunnel(id, core_count, maxCoreQueueLen);
    std::string shmem_region_name = tunnel->getRegionName();
    output->verbose(CALL_INFO, 1, 0, "Base pipe name: %s\n", shmem_region_name.c_str());

    appLauncher = params.find<std::string>("launcher", PINTOOL_EXECUTABLE);

    const uint32_t launch_param_count = (uint32_t) params.find<uint32_t>("launchparamcount", 0);
    const uint32_t pin_arg_count = 29 + launch_param_count;

    execute_args = (char**) malloc(sizeof(char*) * (pin_arg_count + app_argc));

    const uint32_t profileFunctions = (uint32_t) params.find<uint32_t>("profilefunctions", 0);

    output->verbose(CALL_INFO, 1, 0, "Processing application arguments...\n");

    uint32_t arg = 0;
    execute_args[0] = (char*) malloc(sizeof(char) * (appLauncher.size() + 2));
    sprintf(execute_args[0], "%s", appLauncher.c_str());
    arg++;
#if 0
    execute_args[arg++] = const_cast<char*>("-pause_tool");
    execute_args[arg++] = const_cast<char*>("15");
#endif
    execute_args[arg++] = const_cast<char*>("-follow_execv");

    char* param_name_buffer = (char*) malloc(sizeof(char) * 512);

    for(uint32_t aa = 0; aa < launch_param_count; aa++) {
        sprintf(param_name_buffer, "launchparam%" PRIu32, aa);
        std::string launch_p = params.find<std::string>(param_name_buffer, "");

        if("" == launch_p) {
            output->fatal(CALL_INFO, -1, "Error: launch parameter %" PRId32 " is empty string, this must be set to a value.\n",
                    aa);
        }

        execute_args[arg] = (char*) malloc( sizeof(char) * (launch_p.size() + 1) );
        strcpy(execute_args[arg], launch_p.c_str());
        arg++;
    }

    free(param_name_buffer);

    execute_args[arg++] = const_cast<char*>("-t");
    execute_args[arg++] = (char*) malloc(sizeof(char) * (ariel_tool.size() + 1));
    strcpy(execute_args[arg-1], ariel_tool.c_str());
    execute_args[arg++] = const_cast<char*>("-w");

    if( params.find<int>("writepayloadtrace") == 0 ) {
    	execute_args[arg++] = const_cast<char*>("0");
    } else {
    	execute_args[arg++] = const_cast<char*>("1");
    }

    execute_args[arg++] = const_cast<char*>("-p");
    execute_args[arg++] = (char*) malloc(sizeof(char) * (shmem_region_name.length() + 1));
    strcpy(execute_args[arg-1], shmem_region_name.c_str());
    execute_args[arg++] = const_cast<char*>("-v");
    execute_args[arg++] = (char*) malloc(sizeof(char) * 8);
    sprintf(execute_args[arg-1], "%d", verbosity);
    execute_args[arg++] = const_cast<char*>("-t");
    execute_args[arg++] = (char*) malloc(sizeof(char) * 8);
    sprintf(execute_args[arg-1], "%" PRIu32, profileFunctions);
    execute_args[arg++] = const_cast<char*>("-i");
    execute_args[arg++] = (char*) malloc(sizeof(char) * 30);
    sprintf(execute_args[arg-1], "%" PRIu64, (uint64_t) 1000000000);
    execute_args[arg++] = const_cast<char*>("-c");
    execute_args[arg++] = (char*) malloc(sizeof(char) * 8);
    sprintf(execute_args[arg-1], "%" PRIu32, core_count);
    execute_args[arg++] = const_cast<char*>("-s");
    execute_args[arg++] = (char*) malloc(sizeof(char) * 8);
    sprintf(execute_args[arg-1], "%" PRIu32, pin_startup_mode);
    execute_args[arg++] = const_cast<char*>("-m");
    execute_args[arg++] = (char*) malloc(sizeof(char) * 8);
    sprintf(execute_args[arg-1], "%" PRIu32, intercept_mem_allocations);
    execute_args[arg++] = const_cast<char*>("-k");
    execute_args[arg++] = (char*) malloc(sizeof(char) * 8);
    sprintf(execute_args[arg-1], "%" PRIu32, keep_malloc_stack_trace);
    execute_args[arg++] = const_cast<char*>("-u");
    execute_args[arg++] = (char*) malloc(sizeof(char) * (malloc_map_filename.size() + 1));
    strcpy(execute_args[arg-1], malloc_map_filename.c_str());
    execute_args[arg++] = const_cast<char*>("-d");
    execute_args[arg++] = (char*) malloc(sizeof(char) * 8);
    sprintf(execute_args[arg-1], "%" PRIu32, memmgr->getDefaultPool());
    execute_args[arg++] = const_cast<char*>("--");
    execute_args[arg++] = (char*) malloc(sizeof(char) * (executable.size() + 1));
    strcpy(execute_args[arg-1], executable.c_str());

    char* argv_buffer = (char*) malloc(sizeof(char) * 256);
    for(uint32_t aa = 0; aa < app_argc ; ++aa) {
        sprintf(argv_buffer, "apparg%" PRIu32, aa);
        std::string argv_i = params.find<std::string>(argv_buffer, "");

        output->verbose(CALL_INFO, 1, 0, "Found application argument %" PRIu32 " (%s) = %s\n",
                aa, argv_buffer, argv_i.c_str());
        execute_args[arg] = (char*) malloc(sizeof(char) * (argv_i.size() + 1));
        strcpy(execute_args[arg], argv_i.c_str());
        arg++;
    }
    execute_args[arg] = NULL;
    free(argv_buffer);

    const int32_t pin_env_count = params.find<int32_t>("envparamcount", -1);
    if(pin_env_count > -1) {
        char* env_name_buffer = (char*) malloc(sizeof(char) * 256);

        for(int32_t next_env_param = 0; next_env_param < pin_env_count; next_env_param++) {
                sprintf(env_name_buffer, "envparamname%" PRId32 , next_env_param);

                std::string env_name = params.find<std::string>(env_name_buffer, "");

                if("" == env_name) {
                    output->fatal(CALL_INFO, -1, "Parameter: %s environment variable name is empty",
                            env_name_buffer);
                }

                sprintf(env_name_buffer, "envparamval%" PRId32, next_env_param);

                std::string env_value = params.find<std::string>(env_name_buffer, "");

                execute_env.insert(std::pair<std::string, std::string>(
                    env_name, env_value));
        }

        free(env_name_buffer);
    }

    output->verbose(CALL_INFO, 1, 0, "Completed processing application arguments.\n");

    // Remember that the list of arguments must be NULL terminated for execution
    execute_args[(pin_arg_count - 1) + app_argc] = NULL;

    /////////////////////////////////////////////////////////////////////////////////////

    output->verbose(CALL_INFO, 1, 0, "Creating core to cache links...\n");
    cpu_to_cache_links = (SimpleMem**) malloc( sizeof(SimpleMem*) * core_count );

    if (useAllocTracker) {
        output->verbose(CALL_INFO, 1, 0, "Creating core to allocate tracker links...\n");
        cpu_to_alloc_tracker_links = (Link**) malloc( sizeof(Link*) * core_count );
    } else {
        cpu_to_alloc_tracker_links = 0;
    }


    if(opal_enabled) {
        output->verbose(CALL_INFO, 1, 0, "Creating core to Opal links...\n");
        cpu_to_opal_links = (Link**) malloc( sizeof(Link*) * core_count );
    }


    output->verbose(CALL_INFO, 1, 0, "Creating processor cores and cache links...\n");
    cpu_cores = (ArielCore**) malloc( sizeof(ArielCore*) * core_count );

    output->verbose(CALL_INFO, 1, 0, "Configuring cores and cache links...\n");
    char* link_buffer = (char*) malloc(sizeof(char) * 256);
    for(uint32_t i = 0; i < core_count; ++i) {
        sprintf(link_buffer, "cache_link_%" PRIu32, i);

        cpu_cores[i] = new ArielCore(tunnel, NULL, i, maxPendingTransCore, output,
                maxIssuesPerCycle, maxCoreQueueLen, cacheLineSize, this,
                memmgr, perform_checks, params);
        cpu_to_cache_links[i] = dynamic_cast<Interfaces::SimpleMem*>(loadSubComponent("memHierarchy.memInterface", this, params));
        cpu_to_cache_links[i]->initialize(link_buffer, new SimpleMem::Handler<ArielCore>(cpu_cores[i], &ArielCore::handleEvent));

        // Set max number of instructions
        cpu_cores[i]->setMaxInsts(max_insts);

        // optionally wire up links to allocate trackers (e.g. memSieve)
        if (useAllocTracker) {
            sprintf(link_buffer, "alloc_link_%" PRIu32, i);
            cpu_to_alloc_tracker_links[i] = configureLink(link_buffer);
            cpu_cores[i]->setCacheLink(cpu_to_cache_links[i], cpu_to_alloc_tracker_links[i]);
        } else {
            cpu_cores[i]->setCacheLink(cpu_to_cache_links[i], 0);
        }

        if(opal_enabled) {
            sprintf(link_buffer, "opal_link_%" PRIu32, i);
            cpu_to_opal_links[i] = configureLink(link_buffer, new Event::Handler<ArielCore>(cpu_cores[i], &ArielCore::handleInterruptEvent));
            cpu_cores[i]->setOpalLink(cpu_to_opal_links[i]);
            cpu_cores[i]->setOpal();
        }
    }

    free(link_buffer);

    std::string cpu_clock = params.find<std::string>("clock", "1GHz");
    output->verbose(CALL_INFO, 1, 0, "Registering ArielCPU clock at %s\n", cpu_clock.c_str());

    registerClock( cpu_clock, new Clock::Handler<ArielCPU>(this, &ArielCPU::tick ) );

    output->verbose(CALL_INFO, 1, 0, "Clocks registered.\n");

    // Register us as an important component
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    stopTicking = true;

    output->verbose(CALL_INFO, 1, 0, "Completed initialization of the Ariel CPU.\n");
    fflush(stdout);
}


void ArielCPU::init(unsigned int phase)
{
    if ( phase == 0 ) {
        output->verbose(CALL_INFO, 1, 0, "Launching PIN...\n");
        // Init the child_pid = 0, this prevents problems in emergencyShutdown()
        // if forkPINChild() calls fatal (i.e. the child_pid would not be set)
        child_pid = 0;
        child_pid = forkPINChild(appLauncher.c_str(), execute_args, execute_env);
        output->verbose(CALL_INFO, 1, 0, "Returned from launching PIN.  Waiting for child to attach.\n");

        tunnel->waitForChild();
        output->verbose(CALL_INFO, 1, 0, "Child has attached!\n");
    }

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
}

int ArielCPU::forkPINChild(const char* app, char** args, std::map<std::string, std::string>& app_env) {
    // If user only wants to init the simulation then we do NOT fork the binary
    if(Simulation::getSimulation()->getSimulationMode() == Simulation::INIT)
        return 0;

    int next_arg_index = 0;
    int next_line_index = 0;

    char* full_execute_line = (char*) malloc(sizeof(char) * 16384);

    memset(full_execute_line, 0, sizeof(char) * 16384);

    while(NULL != args[next_arg_index]) {
        int copy_char_index = 0;

        if(0 != next_line_index) {
                full_execute_line[next_line_index++] = ' ';
        }

        while('\0' != args[next_arg_index][copy_char_index]) {
                full_execute_line[next_line_index++] = args[next_arg_index][copy_char_index++];
        }

        next_arg_index++;
    }

    full_execute_line[next_line_index] = '\0';

    output->verbose(CALL_INFO, 2, 0, "Executing PIN command: %s\n", full_execute_line);
    free(full_execute_line);

    pid_t the_child;

    // Fork this binary, then exec to get around waiting for
    // child to exit.
    the_child = fork();
    if ( the_child < 0 ) {
        perror("fork");
        output->fatal(CALL_INFO, 1, "Fork failed to launch the traced process. errno = %d, errstr = %s\n", errno, strerror(errno));
    }

    if(the_child != 0) {
        // Set the member variable child_pid in case the waitpid() below fails
        // this allows the fatal process to kill the process and prevent it
        // from becoming a zombie process.  Because as we all know, zombies are
        // bad and eat your brains...
        child_pid = the_child;

        // This is the parent, return the PID of our child process
    /* Wait a second, and check to see that the child actually started */
    sleep(1);
    int pstat;
    pid_t check = waitpid(the_child, &pstat, WNOHANG);
    if ( check > 0 ) {
        // The child process is Stopped or Terminated.
        // Ther are 3 possible results
        if (WIFEXITED(pstat) == true) {
            output->fatal(CALL_INFO, 1,
                    "Launching trace child failed!  Child Exited with status %d\n",
                    WEXITSTATUS(pstat));
        }
        else if (WIFSIGNALED(pstat) == true) {
            output->fatal(CALL_INFO, 1,
                    "Launching trace child failed!  Child Terminated With Signal %d; Core Dump File Created = %d\n",
                    WTERMSIG(pstat), WCOREDUMP(pstat));
        }
        else if (WIFSTOPPED(pstat) == true) {
            output->fatal(CALL_INFO, 1,
                    "Launching trace child failed!  Child Stopped with Signal  %d\n",
                    WSTOPSIG(pstat));
        }
        else {
            output->fatal(CALL_INFO, 1,
                "Launching trace child failed!  Unknown Problem; pstat = %d\n",
                pstat);
        }

    } else if ( check < 0 ) {
        perror("waitpid");
        output->fatal(CALL_INFO, 1,
                "Waitpid returned an error, errno = %d.  Did the child ever even start?\n", errno);
    }
        return (int) the_child;
    } else {
        output->verbose(CALL_INFO, 1, 0, "Launching executable: %s...\n", app);

        if(0 == app_env.size()) {
#if defined(SST_COMPILE_MACOSX)
        char *dyldpath = getenv("DYLD_LIBRARY_PATH");

        if(dyldpath) {
            setenv("PIN_APP_DYLD_LIBRARY_PATH", dyldpath, 1);
            setenv("PIN_DYLD_RESTORE_REQUIRED", "t", 1);
            unsetenv("DYLD_LIBRARY_PATH");
        }
#else
#if defined(HAVE_SET_PTRACER)
        prctl(PR_SET_PTRACER, getppid(), 0, 0 ,0);
#endif // End of HAVE_SET_PTRACER
#endif // End SST_COMPILE_MACOSX (else branch)
            int ret_code = execvp(app, args);
            perror("execve");

            output->verbose(CALL_INFO, 1, 0,
                "Call to execvp returned: %d\n", ret_code);

            output->fatal(CALL_INFO, -1,
                "Error executing: %s under a PIN fork\n",
                app);
        } else {
            char** execute_env_cp = (char**) malloc(sizeof(char*) * (app_env.size() + 1));
            uint32_t next_env_cp_index = 0;

            for(auto env_itr = app_env.begin(); env_itr != app_env.end(); env_itr++) {
                char* execute_env_nv_pair = (char*) malloc(sizeof(char) * (2 +
                        env_itr->first.size() + env_itr->second.size()));

                output->verbose(CALL_INFO, 2, 0, "Env: %s=%s\n",
                        env_itr->first.c_str(), env_itr->second.c_str());

                sprintf(execute_env_nv_pair, "%s=%s", env_itr->first.c_str(),
                        env_itr->second.c_str());

                execute_env_cp[next_env_cp_index] = execute_env_nv_pair;
                next_env_cp_index++;
            }

            execute_env_cp[app_env.size()] = NULL;

            int ret_code = execve(app, args, execute_env_cp);
            perror("execvep");

            output->verbose(CALL_INFO, 1, 0, "Call to execvpe returned %d\n", ret_code);
            output->fatal(CALL_INFO, -1, "Error executing %s under a PIN fork\n", app);
        }
    }

    return 0;
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

ArielCPU::~ArielCPU() {
    // Delete all of the cores
    for(uint32_t i = 0; i < core_count; i++) {
        delete cpu_cores[i];
    }

    delete tunnel;
}

void ArielCPU::emergencyShutdown() {
    tunnel->shutdown(true);
    // If child_pid = 0, dont kill (this would kill all processes of the group)
    if (child_pid != 0) {
        kill(child_pid, SIGKILL);
    }

    /* Ask the cores to finish up.  This should flush logging */
    for(uint32_t i = 0; i < core_count; ++i) {
        cpu_cores[i]->finishCore();
    }
}
