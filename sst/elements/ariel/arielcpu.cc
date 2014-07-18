// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/simulation.h>
#include "arielcpu.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <time.h>

#include <string.h>

using namespace SST::ArielComponent;

ArielCPU::ArielCPU(ComponentId_t id, Params& params) :
	Component(id) {

	int verbosity = params.find_integer("verbose", 0);
	output = new SST::Output("ArielComponent[@f:@l:@p] ",
		verbosity, 0, SST::Output::STDOUT);

	output->verbose(CALL_INFO, 1, 0, "Creating Ariel component...\n");

	core_count = (uint32_t) params.find_integer("corecount", 1);
	output->verbose(CALL_INFO, 1, 0, "Configuring for %" PRIu32 " cores...\n", core_count);

	uint32_t perform_checks = (uint32_t) params.find_integer("checkaddresses", 0);
	output->verbose(CALL_INFO, 1, 0, "Configuring for check addresses = %s\n", (perform_checks > 0) ? "yes" : "no");

	memory_levels = (uint32_t) params.find_integer("memorylevels", 1);
	output->verbose(CALL_INFO, 1, 0, "Configuring for %" PRIu32 " memory levels.\n", memory_levels);

	page_sizes = (uint64_t*) malloc( sizeof(uint64_t) * memory_levels );
	page_counts = (uint64_t*) malloc( sizeof(uint64_t) * memory_levels );

	char* level_buffer = (char*) malloc(sizeof(char) * 256);
	for(uint32_t i = 0; i < memory_levels; ++i) {
		sprintf(level_buffer, "pagesize%" PRIu32, i);
		page_sizes[i] = (uint64_t) params.find_integer(level_buffer, 4096);

		sprintf(level_buffer, "pagecount%" PRIu32, i);
		page_counts[i] = (uint64_t) params.find_integer(level_buffer, 131072);
	}
	free(level_buffer);

	uint32_t default_level = (uint32_t) params.find_integer("defaultlevel", 0);
	uint32_t translateCacheSize = (uint32_t) params.find_integer("translatecacheentries", 4096);

	output->verbose(CALL_INFO, 1, 0, "Creating memory manager, default allocation from %" PRIu32 " memory pool.\n", default_level);
	memmgr = new ArielMemoryManager(memory_levels, 
		page_sizes, page_counts, output, default_level, translateCacheSize);

	uint32_t maxIssuesPerCycle   = (uint32_t) params.find_integer("maxissuepercycle", 1);
	uint32_t maxCoreQueueLen     = (uint32_t) params.find_integer("maxcorequeue", 64);
	uint32_t maxPendingTransCore = (uint32_t) params.find_integer("maxtranscore", 16);
	uint64_t cacheLineSize       = (uint64_t) params.find_integer("cachelinesize", 64);

	/////////////////////////////////////////////////////////////////////////////////////

	shmem_region_name = (char*) malloc(sizeof(char) * 1024);
    const char *tmpdir = getenv("TMPDIR");
    if ( !tmpdir ) tmpdir = "/tmp";
    sprintf(shmem_region_name, "%s/ariel_shmem_%u_%lu_XXXXXX", tmpdir, getpid(), id);
    int fd = mkstemp(shmem_region_name);
    close(fd);

	output->verbose(CALL_INFO, 1, 0, "Base pipe name: %s\n", shmem_region_name);

	/////////////////////////////////////////////////////////////////////////////////////

	std::string ariel_tool = params.find_string("arieltool", "");
	if("" == ariel_tool) {
		output->fatal(CALL_INFO, -1, "The arieltool parameter specifying which PIN tool to run was not specified\n");
	}

	std::string executable = params.find_string("executable", "");
	if("" == executable) {
		output->fatal(CALL_INFO, -1, "The input deck did not specify an executable to be run against PIN\n");
	}

	uint32_t app_argc = (uint32_t) params.find_integer("appargcount", 0);
	output->verbose(CALL_INFO, 1, 0, "Model specifies that there are %" PRIu32 " application arguments\n", app_argc);

	uint32_t pin_startup_mode = (uint32_t) params.find_integer("arielmode", 1);
	uint32_t intercept_multilev_mem = (uint32_t) params.find_integer("arielinterceptcalls", 1);

	switch(intercept_multilev_mem) {
	case 0:
		output->verbose(CALL_INFO, 1, 0, "Interception and re-instrumentation of multi-level memory calls is DISABLED.\n");
		break;
	default:
		output->verbose(CALL_INFO, 1, 0, "Interception and instrumentation of multi-level memory calls is ENABLED\n");
		break;
	}

    tunnel = new ArielTunnel(shmem_region_name, core_count, maxCoreQueueLen, 0.0);

	const char* execute_binary = PINTOOL_EXECUTABLE;
	const uint32_t pin_arg_count = 20;
  	char ** execute_args = (char**) malloc(sizeof(char*) * (pin_arg_count + app_argc));

	output->verbose(CALL_INFO, 1, 0, "Processing application arguments...\n");

    	execute_args[0] = const_cast<char*>(PINTOOL_EXECUTABLE);
    	execute_args[1] = const_cast<char*>("-t");
    	execute_args[2] = (char*) malloc(sizeof(char) * (ariel_tool.size() + 1));
    	strcpy(execute_args[2], ariel_tool.c_str());
    	execute_args[3] = const_cast<char*>("-p");
    	execute_args[4] = (char*) malloc(sizeof(char) * (strlen(shmem_region_name) + 1));
    	strcpy(execute_args[4], shmem_region_name);
    	execute_args[5] = const_cast<char*>("-v");
    	execute_args[6] = (char*) malloc(sizeof(char) * 8);
    	sprintf(execute_args[6], "%d", verbosity);
    	execute_args[7] = const_cast<char*>("-i");
    	execute_args[8] = (char*) malloc(sizeof(char) * 30);
    	sprintf(execute_args[8], "%" PRIu64, (uint64_t) 1000000000);
    	execute_args[9] = const_cast<char*>("-c");
    	execute_args[10] = (char*) malloc(sizeof(char) * 8);
    	sprintf(execute_args[10], "%" PRIu32, core_count);
    	execute_args[11] = const_cast<char*>("-s");
    	execute_args[12] = (char*) malloc(sizeof(char) * 8);
    	sprintf(execute_args[12], "%" PRIu32, pin_startup_mode);
    	execute_args[13] = const_cast<char*>("-m");
    	execute_args[14] = (char*) malloc(sizeof(char) * 8);
    	sprintf(execute_args[14], "%" PRIu32, intercept_multilev_mem);
    	execute_args[15] = const_cast<char*>("-d");
    	execute_args[16] = (char*) malloc(sizeof(char) * 8);
    	sprintf(execute_args[16], "%" PRIu32, default_level);
    	execute_args[17] = const_cast<char*>("--");
    	execute_args[18] = (char*) malloc(sizeof(char) * (executable.size() + 1));
    	strcpy(execute_args[18], executable.c_str());

	char* argv_buffer = (char*) malloc(sizeof(char) * 256);
	for(uint32_t i = (pin_arg_count - 1); i < (pin_arg_count - 1) + app_argc; ++i) {
		sprintf(argv_buffer, "apparg%" PRIu32, i - (pin_arg_count - 1));
		std::string argv_i = params.find_string(argv_buffer, "");

		output->verbose(CALL_INFO, 1, 0, "Found application argument %" PRIu32 " (%s) = %s\n", 
			i - (pin_arg_count - 1), argv_buffer, argv_i.c_str());
		execute_args[i] = (char*) malloc(sizeof(char) * (argv_i.size() + 1));
		strcpy(execute_args[i], argv_i.c_str());
	}
	free(argv_buffer);

	output->verbose(CALL_INFO, 1, 0, "Completed processing application arguments.\n");

	// Remember that the list of arguments must be NULL terminated for execution
	execute_args[(pin_arg_count - 1) + app_argc] = NULL;


	output->verbose(CALL_INFO, 1, 0, "Launching PIN...\n");
	forkPINChild(execute_binary, execute_args);
	output->verbose(CALL_INFO, 1, 0, "Returned from launching PIN.  Waiting for child to attach.\n");

    tunnel->waitForChild();
	output->verbose(CALL_INFO, 1, 0, "Child has attached!\n");
	/////////////////////////////////////////////////////////////////////////////////////

	const std::string tracePrefix = params.find_string("tracePrefix", "");
	if("" == tracePrefix) {
		output->verbose(CALL_INFO, 1, 0, "Core address tracing is not enabled.\n");
	} else {
		output->verbose(CALL_INFO, 1, 0, "Core address tracing is enabled, prefix is set to %s\n", tracePrefix.c_str());
	}

	output->verbose(CALL_INFO, 1, 0, "Creating core to cache links...\n");
	cpu_to_cache_links = (SimpleMem**) malloc( sizeof(SimpleMem*) * core_count );

	output->verbose(CALL_INFO, 1, 0, "Creating processor cores and cache links...\n");
	cpu_cores = (ArielCore**) malloc( sizeof(ArielCore*) * core_count );

	output->verbose(CALL_INFO, 1, 0, "Configuring cores and cache links...\n");
	char* link_buffer = (char*) malloc(sizeof(char) * 256);
	for(uint32_t i = 0; i < core_count; ++i) {
		sprintf(link_buffer, "cache_link_%" PRIu32, i);

		cpu_cores[i] = new ArielCore(tunnel, NULL, i, maxPendingTransCore, output, 
			maxIssuesPerCycle, maxCoreQueueLen, cacheLineSize, this,
			memmgr, perform_checks, tracePrefix);
        cpu_to_cache_links[i] = dynamic_cast<SimpleMem*>(loadModuleWithComponent("memHierarchy.memInterface", this, params));
        cpu_to_cache_links[i]->initialize(link_buffer, new SimpleMem::Handler<ArielCore>(cpu_cores[i], &ArielCore::handleEvent));
		cpu_cores[i]->setCacheLink(cpu_to_cache_links[i]);
	}
	free(link_buffer);

	std::string cpu_clock = params.find_string("clock", "1GHz");
	output->verbose(CALL_INFO, 1, 0, "Registering ArielCPU clock at %s\n", cpu_clock.c_str());

        // TODO: tunnel.setTimeConversion(clockConv);
	//TimeConverter *clockConv = registerClock( cpu_clock, new Clock::Handler<ArielCPU>(this, &ArielCPU::tick ) );

	output->verbose(CALL_INFO, 1, 0, "Clocks registered.\n");

	// Register us as an important component
	registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

	stopTicking = true;

	output->verbose(CALL_INFO, 1, 0, "Completed initialization of the Ariel CPU.\n");
	fflush(stdout);
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

int ArielCPU::forkPINChild(const char* app, char** args) {
	// If user only wants to init the simulation then we do NOT fork the binary
	if(Simulation::getSimulation()->getSimulationMode() == Config::INIT)
		return 0;

	// Set the process group to be the equal to this process (kills child on this
	// process dying)
	setpgid(0, 0);

	pid_t the_child;

	// Fork this binary, then exec to get around waiting for
	// child to exit.
	the_child = fork();

	if(the_child != 0) {
		// This is the parent, return the PID of our child process
		return (int) the_child;
	} else {
		output->verbose(CALL_INFO, 1, 0,
			"Launching executable: %s...\n", app);


#if defined(SST_COMPILE_MACOSX)
        char *dyldpath = getenv("DYLD_LIBRARY_PATH");
        if ( dyldpath ) {
            setenv("PIN_APP_DYLD_LIBRARY_PATH", dyldpath, 1);
            setenv("PIN_DYLD_RESTORE_REQUIRED", "t", 1);
            unsetenv("DYLD_LIBRARY_PATH");
        }
#endif
		int ret_code = execvp(app, args);
		perror("execve");

		output->verbose(CALL_INFO, 1, 0,
			"Call to execvp returned: %d\n", ret_code);

		output->fatal(CALL_INFO, -1, 
			"Error executing: %s under a PIN fork\n",
			app);
	}

	return 0;
}

bool ArielCPU::tick( SST::Cycle_t ) {
	stopTicking = false;
	
	output->verbose(CALL_INFO, 16, 0, "Main processor tick, will issue to individual cores...\n");

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
	delete memmgr;
	delete tunnel;
    unlink(shmem_region_name);
	free(page_sizes);
	free(page_counts);

}

