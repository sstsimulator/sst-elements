// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "ariel.h"

using namespace SST;
using namespace SST::ArielComponent;

int Ariel::create_pinchild(char* prog_binary, char** arg_list) {
	pid_t the_child;

	// Fork this binary, then exec to get around waiting for
	// child to exit.
	the_child = fork();

	if(the_child != 0) {
		// This is the parent, return the PID of our child process
		return (int) the_child;
	} else {
		output->verbose(CALL_INFO, 1, 0,
			"Launching executable: %s...\n", prog_binary);

		int ret_code = execvp(prog_binary, arg_list);
		perror("execve");

		output->verbose(CALL_INFO, 1, 0,
			"Call to execvp returned: %d\n", ret_code);

		output->fatal(CALL_INFO, -1, 0, 0,
			"Error executing: %s under a PIN fork\n",
			prog_binary);
	}

	return 0;
}

Ariel::Ariel(ComponentId_t id, Params& params) :
  Component(id) {

	int verbose = params.find_integer("verbose", 0);
  	output = new Output("SSTArielComponent: ", verbose, 0, SST::Output::STDOUT);

  	named_pipe = (char*) malloc(sizeof(char) * FILENAME_MAX);
  	tmpnam(named_pipe);

  	output->verbose(CALL_INFO, 1, 0, "Named pipe: %s\n", named_pipe);

  	user_binary = params.find_string("executable", "");
  	if(user_binary == "") {
		output->fatal(CALL_INFO, -1, 0, 0, "Did not specify a user executable (parameter: executable)\n");
  	}

  	std::string arieltool = params.find_string("arieltool", "");
 	if(arieltool == "") {
		output->fatal(CALL_INFO, -1, 0, 0, "Model input did not specify a location of a tool to run against Ariel\n");
  	}

	uint32_t memoryLevels = (uint32_t) params.find_integer("memorylevels", 1);
	output->verbose(CALL_INFO, 1, 0, "Model will have: %" PRIu32 " levels of memory\n", memoryLevels);

	uint64_t* pageSizes = (uint64_t*) malloc(sizeof(uint64_t) * memoryLevels);
	uint64_t* pageCounts = (uint64_t*) malloc(sizeof(uint64_t) * memoryLevels);

	for(uint32_t mCount = 0; mCount < memoryLevels; mCount++) {
		char levelBuffer[256];

		sprintf(levelBuffer, "pagesize_%" PRIu32, mCount);
		pageSizes[mCount] = (uint64_t) atol(params.find_string(levelBuffer, "4096").c_str());

		sprintf(levelBuffer, "pagecount_%" PRIu32, mCount);
		pageCounts[mCount] = (uint64_t) atol(params.find_string(levelBuffer, "16384").c_str());

		output->verbose(CALL_INFO, 1, 0, "Configuring memory level %" PRIu32 " for page count of %" PRIu64 " and page size of %" PRIu64 "\n",
			mCount, pageCounts[mCount], pageSizes[mCount]);
	}

  	int app_arg_count = params.find_integer("appargcount", 0);

  	// Get the number of virtual cores, defaults to 1 = serial, allow up to 32 entries per core (these
  	// are shared across the processor as a whole.
  	core_count = (uint32_t) params.find_integer("corecount", 1);
  	output->verbose(CALL_INFO, 1, 0, "Creating processing for %" PRIu32 " cores.\n", core_count);

 	//core_trace_mode = (uint32_t) params.find_integer("tracecores", 0);
  	//output->verbose(CALL_INFO, 1, 0, "Core tracing set to: %" PRIu32 "\n", core_trace_mode);
  	//core_traces = (FILE**) malloc(sizeof(FILE*) * core_count);
  	//if(core_trace_mode > 0) {
	//	for(uint32_t core_counter = 0; core_counter < core_count; core_counter++) {
	//		char core_trace_buffer[256];
	//		sprintf(core_trace_buffer, "core_trace_%" PRIu32, core_counter);
	//
	//		if(core_trace_mode == 1) {
	//			core_traces[core_counter] = fopen(core_trace_buffer, "wb");
	//		} else {
	//			core_traces[core_counter] = fopen(core_trace_buffer, "wt");
	//		}
        //	}
  	//}

  	output->verbose(CALL_INFO, 1, 0, "User model wants application to have %d arguments.\n", app_arg_count);

  	char* execute_binary = PINTOOL_EXECUTABLE;
  	char** execute_args = (char**) malloc(sizeof(char*) * (14 + app_arg_count));

  	execute_args[0] = "pintool";
	execute_args[1] = "-t";
	execute_args[2] = (char*) malloc(sizeof(char) * (arieltool.size() + 1));
  	strcpy(execute_args[2], arieltool.c_str());
  	execute_args[3] = "-p";
  	execute_args[4] = (char*) malloc(sizeof(char) * (strlen(named_pipe) + 1));
  	strcpy(execute_args[4], named_pipe);
  	execute_args[5] = "-v";
  	execute_args[6] = (char*) malloc(sizeof(char) * 8);
  	sprintf(execute_args[6], "%d", verbose);
  	execute_args[7] = "-i";
  	execute_args[8] = (char*) malloc(sizeof(char) * 30);
  	sprintf(execute_args[8], "%" PRIu64, max_inst);
  	execute_args[9] = "-c";
  	execute_args[10] = (char*) malloc(sizeof(char) * 8);
  	sprintf(execute_args[10], "%" PRIu32, core_count);
  	execute_args[11] = "--";
  	execute_args[12] = (char*) malloc(sizeof(char) * (user_binary.size() + 1));
  	strcpy(execute_args[12], user_binary.c_str());

  	char arg_name_buffer[64];

  	output->verbose(CALL_INFO, 2, 0, "Found %d application arguments in model description\n", app_arg_count);

  	for(int app_arg_index = 13; app_arg_index < 13 + app_arg_count; ++app_arg_index) {
		sprintf(arg_name_buffer, "apparg%d", app_arg_index - 13);
        	output->verbose(CALL_INFO, 2, 0, "Searching for application argument in input: %s ... \n", arg_name_buffer);
		std::string app_arg_i = params.find_string(arg_name_buffer);
        	output->verbose(CALL_INFO, 2, 0, "Result is: %s\n", app_arg_i.c_str());

		const int arg_string_size = (int) (app_arg_i.size() + 4);
		output->verbose(CALL_INFO, 4, 0, "Found new application argument: [%s]=[%s], Length=%d, Tool command entry: %d\n", arg_name_buffer, app_arg_i.c_str(),
			arg_string_size, app_arg_index);

		execute_args[app_arg_index] = (char*) malloc(sizeof(char) * (arg_string_size));
        	strcpy(execute_args[app_arg_index], app_arg_i.c_str());
		output->verbose(CALL_INFO, 2, 0, "Completed processing this argument\n");
  	}

  	execute_args[13 + app_arg_count] = NULL;

  	output->verbose(CALL_INFO, 1, 0, "Starting executable: %s\n", execute_binary);
 	output->verbose(CALL_INFO, 1, 0, "Call Arguments: %s %s %s %s %s %s %s %s %s %s %s %s %s (and application arguments)\n",
		execute_args[0], execute_args[1], execute_args[2], execute_args[3],
		execute_args[4], execute_args[5], execute_args[6], execute_args[7],
		execute_args[8], execute_args[9], execute_args[10], execute_args[11],
		execute_args[12]);

  	output->verbose(CALL_INFO, 1, 0, "Launched child PIN process, will connect to PIPE...\n");
  	// Spawn the PIN process
  	create_pinchild(execute_binary, execute_args);

  	pipe_id = (int*) malloc(sizeof(int) * core_count);
  	char* named_pipe_core = (char*) malloc(sizeof(char) * 255);

  	for(uint32_t pipe_counter = 0; pipe_counter < core_count; ++pipe_counter) {
		output->verbose(CALL_INFO, 1, 0, "Generating name for pipe (thread %d)...\n", pipe_counter);
		sprintf(named_pipe_core, "%s-%d", named_pipe, pipe_counter);
		output->verbose(CALL_INFO, 1, 0, "Creating pipe: %s ...\n", named_pipe_core);

  		// We will create a pipe to talk to the PIN child.
  		mkfifo(named_pipe_core, 0666);
  		pipe_id[pipe_counter] = open(named_pipe_core, O_RDONLY | O_NONBLOCK);

		if(pipe_id[pipe_counter] == -1) {
			// File creation failed.
			perror("Unable to create pipe\n");
			output->fatal(CALL_INFO, -1, 0, 0, "Failed to create pipe: %s\n", named_pipe_core);
		} else {
			output->verbose(CALL_INFO, 2, 0, "Successfully created pipe: %s on file ID: %d (for core: %d)\n",
				named_pipe_core, pipe_id[pipe_counter], pipe_counter);
		}
  	}

  	output->verbose(CALL_INFO, 2, 0, "Completed pipe opening, register clock and then will continue.\n");
  	string ariel_clock_rate = params.find_string("clock", "1GHz");
  	output->verbose(CALL_INFO, 1, 0, "Configuring Ariel clock to be: %s\n", ariel_clock_rate.c_str());

	// Register a clock for ourselves
  	registerClock( ariel_clock_rate,
                 new Clock::Handler<Ariel>(this, &Ariel::tick ) );

  	// tell the simulator not to end without us
  	registerAsPrimaryComponent();
  	primaryComponentDoNotEndSim();

  	cache_link = (SST::Link**) malloc(sizeof(SST::Link*) * core_count);

  	for(uint32_t core_counter = 0; core_counter < core_count; ++core_counter) {
		char name_buffer[256];
		sprintf(name_buffer, "cache_link_%d", core_counter);

		output->verbose(CALL_INFO, 2, 0, "Creating a link: %s\n", name_buffer);
	  	cache_link[core_counter] = configureLink(name_buffer, new Event::Handler<Ariel>(this,
                                &Ariel::handleEvent) );
	  	assert(cache_link);
  	}

	output->verbose(CALL_INFO, 1, 0, "Creating processor cores...\n");

	cores = (ArielCore**) malloc(sizeof(ArielCore*) * core_count);
	for(uint32_t i = 0; i < core_count; ++i) {
		cores[i] = new ArielCore(pipe_id[i], cache_link[i], i, 32, output, 2, 64);
	}

	// Configure optional link to DMA engine
  	/*dmaLink = configureLink("dmaLink", new Event::Handler<Ariel>(this, &Ariel::handleDMAEvent));
  if ( NULL != dmaLink ) {
    	output->verbose(CALL_INFO, 2, 0, "DMA Attached\n");
	dma_pending_events = (MemEvent::id_type*) malloc(sizeof(MemEvent::id_type) * core_count);
  } else {
    output->verbose(CALL_INFO, 2, 0, "NO DMA Attached\n");
  }*/

  	output->verbose(CALL_INFO, 1, 0, "Ariel configuration phase is complete.\n");
}

void Ariel::finish() {
  output->verbose(CALL_INFO, 2, 0, "Component is finishing.\n");

  for(uint32_t i = 0; i < core_count; ++i) {
  	close(pipe_id[i]);

//	if(core_trace_mode > 0) {
//		fclose(core_traces[i]);
//	}
  }

  output->verbose(CALL_INFO, 2, 0, "Pipes have been closed, attempting to free resources...\n");

  for(uint32_t i = 0; i < core_count; ++i) {
	char* named_pipe_core = (char*) malloc(sizeof(char) * FILENAME_MAX);
	sprintf(named_pipe_core, "%s-%d", named_pipe, i);

  	output->verbose(CALL_INFO, 2, 0, "Removing named pipe: %s\n", named_pipe_core);

  	// Attempt to remove the pipe
  	remove(named_pipe_core);

	// Free the space we allocated for the name
	free(named_pipe_core);
  }

  /*output->verbose(CALL_INFO, 2, 0, "Component finish completed.\n");
  output->verbose(CALL_INFO, 0, 0, "Insts:            %" PRIu64 "\n", instructions);
  output->verbose(CALL_INFO, 0, 0, "Memory ops:       %" PRIu64 "\n", memory_ops);
  output->verbose(CALL_INFO, 0, 0, "Read ops:         %" PRIu64 "\n", read_ops);
  output->verbose(CALL_INFO, 0, 0, "Split-Read ops:   %" PRIu64 "\n", split_read_ops);
  output->verbose(CALL_INFO, 0, 0, "Write ops:        %" PRIu64 "\n", write_ops);
  output->verbose(CALL_INFO, 0, 0, "Split-Write ops:  %" PRIu64 "\n", split_write_ops);
  output->verbose(CALL_INFO, 0, 0, "Fast mem ops:     %" PRIu64 "\n", fastmem_access_count);
  output->verbose(CALL_INFO, 0, 0, "Std mem ops:      %" PRIu64 "\n", mem_access_count);
  output->verbose(CALL_INFO, 0, 0, "FPge Translates:  %" PRIu64 "\n", fastpage_translation_count);
  output->verbose(CALL_INFO, 0, 0, "Pge Translates:   %" PRIu64 "\n", page_translation_count);
  output->verbose(CALL_INFO, 0, 0, "Time (ns):        %" PRIu64 "\n", getCurrentSimTimeNano());*/
}

Ariel::Ariel() :
    Component(-1)
{
    // for serialization only
}

bool Ariel::tick( Cycle_t thisCycle ) {
	bool foundCoreHalt = false;

	// Cycle over each core, ticking it as needed. If the core halts
	// during this cycle then its time to halt the component
	for(uint32_t i = 0; i < core_count; ++i) {
		cores[i]->tick();

		if(cores[i]->isCoreHalted()) {
			foundCoreHalt = true;
			break;
		}
	}

	return foundCoreHalt;
}

// Element Libarary / Serialization stuff
BOOST_CLASS_EXPORT(Ariel)
