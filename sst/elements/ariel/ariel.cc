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
#define START_DMA 8  // Format: Destination:64 Source:64 Size:32 Tag:32
#define WAIT_DMA 16  // Format: Tag:32
#define ISSUE_TLM_MAP 80 // issue a Two level memory allocation
#define START_INSTRUCTION 32
#define END_INSTRUCTION 64

void Ariel::issue(uint64_t addr, uint32_t length, bool isRead, uint32_t thrID) {
	const uint64_t cache_offset = addr % ((uint64_t) cache_line_size);

	if((cache_offset + length) > cache_line_size) {
		uint64_t cache_left_address = addr;
		uint32_t cache_left_size = cache_line_size - cache_offset;

		// We need data from the next cache line along
		uint64_t cache_right_address = (addr - cache_offset) + cache_line_size;
		uint32_t cache_right_size = (addr + length) % cache_line_size;

		uint64_t phys_cache_left_addr = translateAddress(cache_left_address);
		uint64_t phys_cache_right_addr = translateAddress(cache_right_address);

		output->verbose(CALL_INFO, 2, 0,
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

/*		if((cache_left_size + cache_right_size) != length) {
			std::cout << "cache length=" << length << " left=" << cache_left_size << " right=" << cache_right_size <<
				" addr=" << addr << ", cache_offset=" << cache_offset << std::endl;
			output->fatal(CALL_INFO, -4, 0, 0,
			"Error issuing a cache operation, cache size left %" PRIu32 " != cache size right %" PRIu32 ", length=%" PRIu32 "\n",
			cache_left_size, cache_right_size, length);
		}
*/
		// Produce operations for the lower and upper halves of the transaction
		if((cache_left_size + cache_right_size) == length) {
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
		}
	} else {
		uint64_t physical_addr = translateAddress(addr);

		output->verbose(CALL_INFO, 2, 0,
			"Issue non-split cache line operation: A=%" PRIu64 ", Length=%" PRIu32
			", PhysA=%" PRIu64 ", W/R=%s, TID=%" PRIu32 "\n",
			addr, length, physical_addr,
			isRead ? "READ" : "WRITE", thrID);

		MemEvent *ev = new MemEvent(this, physical_addr, isRead ? ReadReq : WriteReq);
		ev->setSize(length);

		pending_requests.insert( std::pair<MemEvent::id_type, MemEvent*>(ev->getID(), ev) );

		// Check we are not issuing to a random core.
		if(thrID > core_count) {
			output->fatal(CALL_INFO, -4, 0, 0,
			"Error: threadID %" PRIu32 " is greater than number of cores: %" PRIu32 "\n",
			thrID, core_count);
		}

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

	return 0;
}

uint64_t Ariel::translateAddress(uint64_t addr) {
	// Get address offset for this page.
	uint64_t addr_offset = addr % page_size;

	// Get the start of the virtual page for this address.
	uint64_t addr_vpage_start = (addr - (addr_offset));

	std::map<uint64_t, uint64_t>::iterator fastaddr_entry = page_table_fastmem->find(addr_vpage_start);
	std::map<uint64_t, uint64_t>::iterator addr_entry = page_table->find(addr_vpage_start);

	if(fastaddr_entry != page_table_fastmem->end()) {
		// We found this page in fast memory
		output->verbose(CALL_INFO, 2, 0, "Fast memory mapped: %" PRIu64 " to physical: %" PRIu64 "\n",
			addr, (fastaddr_entry->second + addr_offset));

		// Return the fast address map
		return (fastaddr_entry->second + addr_offset);
	}

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
  printf("In Ariel creating output...\n");
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

  fastmem_size = (uint64_t) atol(params.find_string("fastmempagecount", "1048576").c_str());
  output->verbose(CALL_INFO, 1, 0, "Mapping fast memory size to %" PRIu64 " page entries.\n", fastmem_size);

  cache_line_size = (uint64_t) atol(params.find_string("cachelinsize", "64").c_str());
  output->verbose(CALL_INFO, 1, 0, "Cache line size configured for: %" PRIu64 "bytes.\n", cache_line_size);

  // Allocate free pages into the fast memory set, these are going to be used as a free pool
  free_pages_fastmem = new std::vector<uint64_t>();
  for(uint32_t page_counter = 0; page_counter < fastmem_size; page_counter++) {
	free_pages_fastmem->push_back((uint64_t)(page_counter * page_size));
  }

  // Record the mallocs so we can free later (remember two level memory is precious)
  fastmem_alloc_to_size = new std::map<uint64_t, uint64_t>();

  // Allocate the (empty) page tables
  page_table_fastmem = new std::map<uint64_t, uint64_t>();
  page_table = new std::map<uint64_t, uint64_t>();
  // Set the default pool (in DDR) to come from address space beyond the fast memory size
  next_free_page_start = (fastmem_size + 1) * page_size;

  int app_arg_count = params.find_integer("appargcount", 0);

  // Get the number of virtual cores, defaults to 1 = serial, allow up to 32 entries per core (these
  // are shared across the processor as a whole.
  core_count = (uint32_t) params.find_integer("corecount", 1);
  output->verbose(CALL_INFO, 1, 0, "Creating processing for %" PRIu32 " cores.\n", core_count);

  char* execute_binary = PINTOOL_EXECUTABLE;
  char** execute_args = (char**) malloc(sizeof(char*) * 14 + app_arg_count);

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
	std::string app_arg_i = params.find_string(arg_name_buffer);

	output->verbose(CALL_INFO, 4, 0, "Found new application argument: [%s]\n", app_arg_i.c_str());

	execute_args[app_arg_index] = (char*) malloc(sizeof(char) * (app_arg_i.size() + 1));
        strcpy(execute_args[app_arg_index], app_arg_i.c_str());
  }

  execute_args[13 + app_arg_count] = NULL;

  output->verbose(CALL_INFO, 1, 0, "Starting executable: %s\n", execute_binary);
  output->verbose(CALL_INFO, 1, 0, "Call Arguments: %s %s %s %s %s %s %s %s %s %s %s %s %s (and application arguments)\n",
	execute_args[0], execute_args[1], execute_args[2], execute_args[3],
	execute_args[4], execute_args[5], execute_args[6], execute_args[7],
	execute_args[8], execute_args[9], execute_args[10], execute_args[11],
	execute_args[12]);

  max_transactions = (uint32_t) params.find_integer("maxtransactions", core_count * 32);
  output->verbose(CALL_INFO, 1, 0, "Configuring maximum transactions for %" PRIu32 ".\n", max_transactions);

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

  pending_transaction = NULL;

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

  for(uint32_t core_counter = 0; core_counter < core_count; ++core_counter) {
	char name_buffer[256];
	sprintf(name_buffer, "cache_link_%d", core_counter);

	output->verbose(CALL_INFO, 2, 0, "Creating a link: %s\n", name_buffer);
  	cache_link[core_counter] = configureLink(name_buffer, new Event::Handler<Ariel>(this,
                                &Ariel::handleEvent) );
  	assert(cache_link);
  }

  // Configure optional link to DMA engine
  dmaLink = configureLink("dmaLink");
  if ( NULL != dmaLink ) {
    output->verbose(CALL_INFO, 2, 0, "DMA Attached\n");
  } else {
    output->verbose(CALL_INFO, 2, 0, "NO DMA Attached\n");
  }
}

void Ariel::allocateInFastMemory(uint64_t vAddr, uint64_t length) {
	// We need to map a physical address range into fast memory.
	uint64_t page_count = length / page_size;

	// Catch a nonaligned page allocation, what a horrible mess.
	if(page_count * page_size != length) {
		page_count = page_count + 1;
	}

	output->verbose(CALL_INFO, 1, 0, "Allocation request to allocator (fast mem): address=%" PRIu64 ", length=%" PRIu64 ", pages=%" PRIu64 "\n",
		vAddr, length, page_count);

	if(free_pages_fastmem->size() < page_count) {
		// We do not have enough memory, this is a fault
		output->fatal(CALL_INFO, -8, 0, 0,
			"Error - run out of fast memory space for a request\n");
		exit(-8);
	}

	// Keep track of our allocations as we will want to free them at some point
	fastmem_alloc_to_size->insert( std::pair<uint64_t, uint64_t>(vAddr, length) );

	std::vector<uint64_t>::iterator fast_mem_free_page_itr = free_pages_fastmem->begin();
	for(uint64_t page_counter = 0; page_counter < page_count; page_counter++) {
		output->verbose(CALL_INFO, 4, 0, "Mapping virtual %" PRIu64 " to physical: %" PRIu64 "\n",
			(vAddr + page_counter * page_size), *fast_mem_free_page_itr);

		// Allocate into the table to a new address and then remove from our free list
		page_table_fastmem->insert( std::pair<uint64_t, uint64_t>(vAddr + (page_counter * page_size), *fast_mem_free_page_itr) );
		free_pages_fastmem->erase(fast_mem_free_page_itr);
		++fast_mem_free_page_itr;
	}

	output->verbose(CALL_INFO, 2, 0, "Fast memory allocation is complete, pages remaining: %" PRIu32 "\n",
		(uint32_t) free_pages_fastmem->size());
}

void Ariel::finish() {
  output->verbose(CALL_INFO, 2, 0, "Component is finishing.\n");

  for(uint32_t i = 0; i < core_count; ++i) {
  	close(pipe_id[i]);
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

/*	// Check for events from the optional DMA unit
	if (NULL != dmaLink) {
	  Event *e;
	  while ( NULL != (e = dmaLink->recv()) ) {
	    MemHierarchy::DMACommand *cmd = dynamic_cast<MemHierarchy::DMACommand *>(e);
	    if (NULL != cmd) {
	      MemEvent::id_type cmdID = cmd->getID();
	      IDToTagMap_t::iterator outI = outstandingDMACmds.find(cmdID);
	      if (outI != outstandingDMACmds.end()) {
		uint32_t tag = outI->second;
		outstandingDMACmds.erase(outI);
		outstandingDMATags.erase(tag);
	      } else {
		output->fatal(CALL_INFO, -1, 0, 0, "Error: Recienved a DMA command completion for a DMA command we never issued\n");
	      }
	    } else {
	      output->fatal(CALL_INFO, -1, 0, 0, "Error: Unexpected Event Type on DMA Command Link.\n");
	    }
	  }
	}
*/
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
		// Configure the polling set to read from the many thread pipes we have.
		struct pollfd pipe_pollfd[core_count];
		for(uint32_t core_counter = 0; core_counter < core_count; ++core_counter) {
			pipe_pollfd[core_counter].fd = pipe_id[core_counter];
			pipe_pollfd[core_counter].events = POLLIN;
		}

		output->verbose(CALL_INFO, 4, 0, "Polling for new data from the PIN tools...\n");
		// Poll on the files remembering we wait
		poll(pipe_pollfd, (unsigned int) core_count, 10);

		output->verbose(CALL_INFO, 4, 0, "Poll operation has completed.\n");

		for(uint32_t core_counter = 0; core_counter < core_count; ++core_counter) {
			uint8_t command = 0;

			if(pipe_pollfd[core_counter].revents & POLLIN) {
				output->verbose(CALL_INFO, 4, 0, "Data available for core: %d\n", core_counter);

				read(pipe_id[core_counter], &command, sizeof(command));

				if(command == PERFORM_EXIT) {
					output->verbose(CALL_INFO, 8, 0,
						"Read an exit command from pipe stream\n");

					// Exit
					primaryComponentOKToEndSim();
					return true;
				}

				if(command == ISSUE_TLM_MAP) {
					uint64_t virtualAddress = 0;
					uint64_t allocationLength = 0;

					read(pipe_id[core_counter], &virtualAddress, sizeof(virtualAddress));
					read(pipe_id[core_counter], &allocationLength, sizeof(allocationLength));

					output->verbose(CALL_INFO, 1, 0, "Request to perform a two-level memory mapping from: %" PRIu64 " of length: %" PRIu64 "\n", virtualAddress, allocationLength);

					allocateInFastMemory(virtualAddress, allocationLength);

					// We are done with this cycle
					break;
				}

				if(command == START_INSTRUCTION) {
					output->verbose(CALL_INFO, 8, 0,
						"Read a start instruction\n");
					instructions++;
				}

				// Read the next instruction
				read(pipe_id[core_counter], &command, sizeof(command));

				while(command != END_INSTRUCTION) {
					if(command == PERFORM_READ) {
						uint64_t addr;
						read(pipe_id[core_counter], &addr, sizeof(addr));
						uint32_t read_size;
						read(pipe_id[core_counter], &read_size, sizeof(read_size));

						if(read_size > (uint32_t) cache_line_size) {
							output->verbose(CALL_INFO, 2, 0, "Length of a read is larger than a cache line: %" PRIu32 "\n",
								read_size);
						}

						issue(addr, read_size, true, (uint32_t) core_counter);

						memory_ops++;
						read_ops++;
					} else if (command == PERFORM_WRITE) {
						uint64_t addr;
						read(pipe_id[core_counter], &addr, sizeof(addr));
						uint32_t write_size;
						read(pipe_id[core_counter], &write_size, sizeof(write_size));

						if(write_size > (uint32_t) cache_line_size) {
							output->verbose(CALL_INFO, 2, 0, "Length of write is larger than a cache line: %" PRIu32 "\n",
								write_size);
						}

						issue(addr, write_size, false, (uint32_t) core_counter);

						memory_ops++;
						write_ops++;
					} else if (command == START_DMA) {
/*					  if (NULL != dmaLink) {
					    // collect arguments
					    uint64_t dst, src;
					    read(pipe_id[core_counter], &dst, sizeof(dst));
					    read(pipe_id[core_counter], &src, sizeof(src));
					    uint32_t size;
					    read(pipe_id[core_counter], &size, sizeof(size));
					    uint32_t tag;
					    read(pipe_id[core_counter], &tag, sizeof(tag));

					    // construct and send DMA command
					    MemHierarchy::DMACommand *cmd = new MemHierarchy::DMACommand(this, dst, src, size);
					    MemEvent::id_type cmdID = cmd->getID();
					    if (outstandingDMATags.find(tag) == outstandingDMATags.end()) {
					      outstandingDMATags.insert(tagToIDMap_t::value_type(tag, cmdID));
					      outstandingDMACmds.insert(IDToTagMap_t::value_type(cmdID, tag));
					    } else {
					      output->fatal(CALL_INFO, -1, 0, 0, "Error: Trying to start DMA for existing tag (%d)", tag);
					    }
					    dmaLink->send(cmd);
					  } else {
					    output->fatal(CALL_INFO, -1, 0, 0, "Error: no DMA unit configured / connected");
					  }
*/
					} else if (command == WAIT_DMA) {
/*					  if (NULL != dmaLink) {
					    uint32_t tag;
					    read(pipe_id[core_counter], &tag, sizeof(tag));
					    // search for tag. 
					    if (outstandingDMATags.find(tag) == outstandingDMATags.end()) {
					      // couldn't find tag. Ergo, the DMA has finished (or was never started)
					    } else {
					      // Tag found. Ergo, the DMA has NOT finished
					    }
					  } else {
					    output->fatal(CALL_INFO, -1, 0, 0, "Error: no DMA unit configured / connected");
					  }
*/
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

					read(pipe_id[core_counter], &command, sizeof(command));
				}
			} else {
				output->verbose(CALL_INFO, 2, 0, "No data is available for core %" PRIu32 "\n",
					core_counter);
			}
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
    printf("Creating ariel..\n");
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
