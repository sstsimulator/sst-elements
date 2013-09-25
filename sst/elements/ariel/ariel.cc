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

#define PERFORM_EXIT 1
#define PERFORM_READ 2
#define PERFORM_WRITE 4
#define START_INSTRUCTION 32
#define END_INSTRUCTION 64

void Ariel::issue(uint64_t addr, uint32_t length, bool isRead, uint32_t thrID) {
	uint64_t cache_offset = addr % cache_line_size;

	if((cache_offset + length) > cache_line_size) {
		uint64_t cache_left_address = addr;
		uint32_t cache_left_size = cache_line_size - cache_offset;

		// We need data from the next cache line along
		uint64_t cache_right_address = (addr - cache_offset) + cache_line_size;
		uint32_t cache_right_size = (addr + length) % cache_line_size;


		uint64_t phys_cache_left_addr = translateAddress(cache_left_address);
		uint64_t phys_cache_right_addr = translateAddress(cache_right_address);

		output->verbose(CALL_INFO, 1, 0,
			"Issue split-cache line operation A=%" PRIu64 ", Length=%" PRIu32
			", A_L=%" PRIu64 ", S_L=%" PRIu32 ", A_R=%" PRIu64 ", S_R=%" PRIu32
			", PhysA_L=%" PRIu64 ", PhysA_R=%" PRIu64 ", P_L=%" PRIu64 ", P_R=%" PRIu64 
			", C_L=%" PRIu64 ", C_R=%" PRIu64 ", W/R=%s, TID=%" PRIu32 "\n",
			addr, length, cache_left_address, cache_left_size,
			cache_right_address, cache_right_size,
			phys_cache_left_addr, phys_cache_right_addr,
			phys_cache_left_addr / page_size,
			phys_cache_right_addr / page_size,
			phys_cache_left_addr / cache_line_size,
			phys_cache_right_addr / cache_line_size,
			isRead ? "READ" : "WRITE", thrID);

		assert((cache_left_size + cache_right_size) == length);

		// Produce operations for the lower and upper halves of the transaction
		MemEvent *e_lower = new MemEvent(this, phys_cache_left_addr, isRead ? ReadReq : WriteReq);
               	e_lower->setSize(cache_left_size);

               	MemEvent *e_upper = new MemEvent(this, phys_cache_right_addr, isRead ? ReadReq : WriteReq);
                e_upper->setSize(cache_right_size);

		// Add transactions to our pending list for matching later
		pending_requests.insert( std::pair<MemEvent::id_type, MemEvent*>(e_lower->getID(), e_lower) );
		pending_requests.insert( std::pair<MemEvent::id_type, MemEvent*>(e_upper->getID(), e_upper) );

		assert(thrID < core_count);
		cache_link[thrID]->send(e_lower);
		cache_link[thrID]->send(e_upper);

		if(isRead) {
			split_read_ops++;
		} else {
			split_write_ops++;
		}
	} else {
		uint64_t physical_addr = translateAddress(addr);

		output->verbose(CALL_INFO, 1, 0,
			"Issue non-split cache line operation: A=%" PRIu64 ", Length=%" PRIu32
			", PhysA=%" PRIu64 ", W/R=%s, TID=%" PRIu32 "\n",
			addr, length, physical_addr,
			isRead ? "READ" : "WRITE", thrID);

		MemEvent *ev = new MemEvent(this, physical_addr, isRead ? ReadReq : WriteReq);
		ev->setSize(length);

		pending_requests.insert( std::pair<MemEvent::id_type, MemEvent*>(ev->getID(), ev) );

		cache_link[thrID]->send(ev);
	}
}

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

uint64_t Ariel::translateAddress(uint64_t addr) {
	// Get address offset for this page.
	uint64_t addr_offset = addr % page_size;

	// Get the start of the virtual page for this address.
	uint64_t addr_vpage_start = (addr - (addr_offset));

	std::map<uint64_t, uint64_t>::iterator addr_entry = page_table->find(addr_vpage_start);

	if(addr_entry == page_table->end()) {
		output->verbose(CALL_INFO, 2, 0, "Allocating virtual page: %" PRIu64 " to physical page: %" PRIu64 "\n",
			addr_vpage_start, next_free_page_start);
		page_table->insert( std::pair<uint64_t, uint64_t>(addr_vpage_start, next_free_page_start) );
		next_free_page_start += page_size;

		addr_entry = page_table->find(addr_vpage_start);
	}

	output->verbose(CALL_INFO, 4, 0, "Mapped: %" PRIu64 " virtual to physical %" PRIu64 "\n", addr, 
		(addr_entry->second + addr_offset));
	return (addr_entry->second + addr_offset);
}

Ariel::Ariel(ComponentId_t id, Params& params) :
  Component(id) {

  int verbose = params.find_integer("verbose", 0);
  output = new Output("SSTArielComponent: ", verbose, 0, SST::Output::STDOUT);

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

  page_size = (uint64_t) atol(params.find_string("pagesize", "4096").c_str());
  output->verbose(CALL_INFO, 1, 0, "Page size configured for: %" PRIu64 "bytes.\n", page_size);

  cache_line_size = (uint64_t) atol(params.find_string("cachelinsize", "64").c_str());
  output->verbose(CALL_INFO, 1, 0, "Cache line size configured for: %" PRIu64 "bytes.\n", cache_line_size);

  page_table = new std::map<uint64_t, uint64_t>();
  next_free_page_start = page_size;

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

  pending_transaction = NULL;

  // Get the number of virtual cores, defaults to 1 = serial, allow up to 32 entries per core (these
  // are shared across the processor as a whole.
  core_count = (uint32_t) params.find_integer("corecount", 1);
  max_transactions = (uint32_t) params.find_integer("maxtransactions", core_count * 32);

  // Register a clock for ourselves
  registerClock( "1GHz",
                 new Clock::Handler<Ariel>(this, &Ariel::tick ) );

  // tell the simulator not to end without us
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  memory_ops = 0;
  read_ops = 0;
  write_ops = 0;
  instructions = 0;
  split_read_ops = 0;
  split_write_ops = 0;

  cache_link = (SST::Link**) malloc(sizeof(SST::Link*) * core_count);

  for(int core_counter = 0; core_counter < core_count; ++core_counter) {
	char name_buffer[256];
	sprintf(name_buffer, "cache_link_%d", core_counter);

	output->verbose(CALL_INFO, 2, 0, "Creating a link: %s\n", name_buffer);
  	cache_link[core_counter] = configureLink(name_buffer, new Event::Handler<Ariel>(this,
                                &Ariel::handleEvent) );
  	assert(cache_link);
  }
}

void Ariel::finish() {
  output->verbose(CALL_INFO, 2, 0, "Component is finishing.\n");
  close(pipe_id);

  output->verbose(CALL_INFO, 2, 0, "Removing named pipe: %s\n", named_pipe);
  // Attempt to remove the pipe
  remove(named_pipe);

  output->verbose(CALL_INFO, 2, 0, "Component finish completed.\n");
  output->verbose(CALL_INFO, 0, 0, "Insts:            %" PRIu64 "\n", instructions);
  output->verbose(CALL_INFO, 0, 0, "Memory ops:       %" PRIu64 "\n", memory_ops);
  output->verbose(CALL_INFO, 0, 0, "Read ops:         %" PRIu64 "\n", read_ops);
  output->verbose(CALL_INFO, 0, 0, "Split-Read ops:   %" PRIu64 "\n", split_read_ops);
  output->verbose(CALL_INFO, 0, 0, "Write ops:        %" PRIu64 "\n", write_ops);
  output->verbose(CALL_INFO, 0, 0, "Split-Write ops:  %" PRIu64 "\n", split_write_ops);

}

Ariel::Ariel() :
    Component(-1)
{
    // for serialization only
}

void Ariel::handleEvent(Event *ev)
{
	output->verbose(CALL_INFO, 4, 0, "Event being handled in Ariel component.\n");

	MemEvent *event = dynamic_cast<MemEvent*>(ev);
        if (event) {
		output->verbose(CALL_INFO, 4, 0, "Event successfully cast into memory event type.\n");

                // Branden says to ignore invalidates
               	if ( event->getCmd() == Invalidate ) {
                       	delete event;
                       	return;
               	}

		// Get the ID the event responds to.
		MemEvent::id_type ev_id = event->getResponseToID();
		std::map<MemEvent::id_type, MemEvent*>::iterator pending_itr = pending_requests.find(ev_id);

		if(pending_itr != pending_requests.end()) {
			output->verbose(CALL_INFO, 2, 0, "Memory event to address %" PRIu64 " located in pending transactions, will be removed from list.\n",
				(uint64_t) event->getAddr());
			pending_requests.erase(ev_id);

			// Delete the event so we free up memory, we are done processing this one.
			delete event;
		}
	} else {
		output->fatal(CALL_INFO, -1, 0, 0, "Unknown event type received by Ariel.\n");
	}
}

bool Ariel::tick( Cycle_t ) {
	output->verbose(CALL_INFO, 2, 0, "Clock tick (is transaction pending? %s\n", (pending_transaction == NULL) ? "YES" : "NO");
	output->verbose(CALL_INFO, 2, 0, "Pending Transaction size is: %" PRIu32 " elements\n", (uint32_t) pending_requests.size());

	if(pending_transaction != NULL) {
		if(pending_requests.size() < max_transactions) {
			assert(pending_transaction_core < core_count);
			output->verbose(CALL_INFO, 4, 0, "Found a pending transaction on core %" PRIu32 " pushing to cache.", pending_transaction_core);

			// Push pending transaction into the queue and then reset so
			// we can read another next cycle round.
			cache_link[pending_transaction_core]->send(pending_transaction);
			pending_transaction = NULL;
		}
        } else if( pending_requests.size() >= max_transactions) {
		output->verbose(CALL_INFO, 2, 0, "Pending transactions has reached limit of %" PRIu32 "\n", max_transactions);
	} else if( pending_requests.size() < max_transactions) {
		uint8_t command = 0;
		read(pipe_id, &command, sizeof(command));

		if(command == PERFORM_EXIT) {
			output->verbose(CALL_INFO, 1, 0,
				"Read an exit command from pipe stream\n");

			// Exit
			primaryComponentOKToEndSim();
			return true;
		}

		if(command == START_INSTRUCTION) {
			output->verbose(CALL_INFO, 1, 0,
				"Read a start instruction\n");
			instructions++;
		}

		// Read the next instruction
		read(pipe_id, &command, sizeof(command));

		while(command != END_INSTRUCTION) {
			if(command == PERFORM_READ) {
				uint64_t addr;
				read(pipe_id, &addr, sizeof(addr));
				uint32_t read_size;
				read(pipe_id, &read_size, sizeof(read_size));
				uint32_t thrID;
				read(pipe_id, &thrID, sizeof(thrID));

				issue(addr, read_size, true, thrID);

				memory_ops++;
			read_ops++;
			} else if (command == PERFORM_WRITE) {
				uint64_t addr;
				read(pipe_id, &addr, sizeof(addr));
				uint32_t write_size;
				read(pipe_id, &write_size, sizeof(write_size));
				uint32_t thrID;
				read(pipe_id, &thrID, sizeof(thrID));

				issue(addr, write_size, false, thrID);

				memory_ops++;
				write_ops++;
			} else {
				// So PIN may occassionally not get instrumentation correct and we get
				// instruction records nested inside each other :(.
				if(command == START_INSTRUCTION) {
					instructions++;
				} else {
					output->fatal(CALL_INFO, -1, 0, 0,
						"Error: unknown type of operation: %d\n", command);
				}
			}

			read(pipe_id, &command, sizeof(command));
		}
	}

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
