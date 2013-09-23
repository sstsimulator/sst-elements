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
}

Ariel::Ariel(ComponentId_t id, Params& params) :
  Component(id) {

  int verbose = params.find_integer("verbose", 0);
  output = new Output("Ariel ", verbose, 0, SST::Output::STDOUT);

  // get the maximum number of instructions we can run using Ariel
  max_inst = (uint64_t) atol(params.find_string("maxinst", "10000000000").c_str());

  output->verbose(CALL_INFO, 1, 0, "Max instructions: %" PRIu64 " inst.\n", max_inst);

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

  char* execute_binary = PINTOOL_EXECUTABLE;
  char** execute_args = (char**) malloc(sizeof(char*) * 12);

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
  execute_args[9] = "--";
  execute_args[10] = (char*) malloc(sizeof(char) * (user_binary.size() + 1));
  strcpy(execute_args[10], user_binary.c_str());
  execute_args[11] = NULL;

  output->verbose(CALL_INFO, 1, 0, "Starting executable: %s\n", execute_binary);
  output->verbose(CALL_INFO, 1, 0, "Call Arguments: %s %s %s %s %s %s %s %s %s %s %s\n",
	execute_args[0], execute_args[1], execute_args[2], execute_args[3],
	execute_args[4], execute_args[5], execute_args[6], execute_args[7],
	execute_args[8], execute_args[9], execute_args[10]);

  // Spawn the PIN process
  create_pinchild(execute_binary, execute_args);

  output->verbose(CALL_INFO, 1, 0, "Launched child PIN process, will connect to PIPE...\n");

  // We will create a pipe to talk to the PIN child.
  mkfifo(named_pipe, 0666);
  pipe_id = open(named_pipe, O_RDONLY);

  output->verbose(CALL_INFO, 2, 0, "Passed the pipe opening, register clock and then will continue.\n");

  // Register a clock for ourselves
  registerClock( "1GHz",
                 new Clock::Handler<Ariel>(this, &Ariel::tick ) );

  // tell the simulator not to end without us
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}

Ariel::Ariel() :
    Component(-1)
{
    // for serialization only
}

void Ariel::handleEvent(Event *ev)
{

}

bool Ariel::tick( Cycle_t ) {
	output->verbose(CALL_INFO, 2, 0, "Clock tick.\n");

	return false;
}

// Element Libarary / Serialization stuff
BOOST_CLASS_EXPORT(Ariel)

static Component*
create_ariel(SST::ComponentId_t id, 
                  SST::Params& params)
{
    return new Ariel( id, params );
}

static const ElementInfoComponent components[] = {
    { "ariel",
      "PIN-based Memory Tracing Component",
      NULL,
      create_ariel
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo ariel_eli = {
        "ariel",
        "PIN-based Memory Tracing Component",
        components,
    };
}
