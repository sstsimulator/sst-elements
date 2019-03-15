// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */



#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/elementinfo.h>

#include <sst/core/output.h>

#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>

#include "NVM_Params.h"
#include "NVM_DIMM.h"


using namespace std;
using namespace SST;

using namespace SST::MessierComponent;

namespace SST {
	namespace MessierComponent {

		class NVM_PARAMS;

		class Messier : public SST::Component {
			public:

                SST_ELI_REGISTER_COMPONENT(
                    Messier,
                    "Messier",
                    "Messier",
                    SST_ELI_ELEMENT_VERSION(1,0,0),
                    "Messier Component",
                    COMPONENT_CATEGORY_MEMORY
                )

                SST_ELI_DOCUMENT_PARAMS(
                    {"clock",              "Internal Controller Clock Rate.", "1.0 Ghz"},
                    {"memory_clock",              "NVM Chips Clock Rate.", "1.0 Ghz"},
                    {"size",         "The size of the whole NVM-Based DIMM in KBs", "8388608"},
                    {"write_buffer_size",            "The size of the internal write buffer (# entries) ", "64"},
                    {"max_requests",              "This determines the max number of requests can be buffered at the NVMDIMM", "64"},
                    {"max_outstanding", "This determines the max number of outstanding/executing requests inside the NVMDIMM", "16"},
                    {"max_current_weight", "Determines the power limit, which in turn limits the outstanding requests", "120"},
                    {"write_weight", "Determines the average power weight of a currently executing write operation", "15"},
                    {"read_weight", "Determines the average power weight of a currently executing read operation", "3"},
                    {"tCMD", "The time needed to send a command from the controller to the NVM-Chip (in controller cycles)", "1"},
                    {"tCL", "The time needed to read a word from an already activated row (in controller cycles)", "70" },
                    {"tCL_W", "The time needed to write a word completely to the PCM Chip (in controller cycles)", "1000"},
                    {"tRCD", "The time needed to activate a row in PCM chips (in controller cycles)", "300"},
                    {"tBURST", "The time needed for submitting a data burst on the bus", "7"},
                    {"device_width", "This determines the NVM-Chip output width", "8"},
                    {"num_devices", "The number of NVM-Chip devices", "8"},
                    {"num_ranks", "The number of ranks", "1"},
                    {"num_banks", "The number of banks on each rank", "16"},
                    {"cache_enabled", "If internal cache is enabled or not", "0"},
                    {"modulo", "If internal modulo scheduling is enabled or not", "0"},
                    {"modulo_unit", "The unit of module, if N, this means for each N reads, we service a write", "4"},
                    {"cache_persistent", "There is enough power to write-back the cache", "0"},
                    {"cache_latency", "The cache latency", "0"},
                    {"cache_size", "The cache size in KBs", "0"},
                    {"cache_bs", "The cache block size", "0"},
                    {"cache_assoc", "The cache associativity", "0"},
                    {"row_buffer_size", "The row buffer size in bytes", "8192"},
                    {"flush_th", "This determines the percentage of max number of entries in write buffer that triggers flushing it", "50"},
                    {"flush_th_low", "This determines the percentage of the low threshold number of entries", "30"},
                    {"max_writes", "This determines the maximum number of concurrent writes at NVM chips (not including those on buffer", "1"},
                    {"cacheline_interleaving", "This determines if cacheline interleaving is used (1) or bank interleaving (0) ", "1"},
                    {"adaptive_writes", "This indicates that the writes flushing mode: 0 means naive interleaving", "0"},
                    {"write_cancel", "This indicates that the write cancellation optimization: 0 means not enabled", "0"},
                    {"write_cancel_th", "This indicates that the write cancellation threshold: 0 means dynamic", "0"},
                    {"group_size", "This indicates the number of banks in each group, to be locked when draining", "0"},
                    {"lock_period", "This indicates the period of locking a group in cycles", "10000"}
                )

                SST_ELI_DOCUMENT_STATISTICS(
                    { "reads", "Determine the number of reads", "reads", 1},
                    { "writes", "Determine the number of writes", "writes", 1},
                    { "avg_time", "The average time spent on each read request", "cycles", 3},
                    { "histogram_idle", "The histogram of cycles length while controller is idle", "cycles",1}
                )

                SST_ELI_DOCUMENT_PORTS(
                    {"bus", "Link to the socket controller", {}}
                )

				Messier( SST::ComponentId_t id, SST::Params& params); 
				void setup()  { };
				void finish() {DIMM->finish();};
				void handleEvent(SST::Event* event) {};
				bool tick(SST::Cycle_t x);

				void parser(NVM_PARAMS * nvm, SST::Params& params);				


			private:
				Messier();  // for serialization only
				Messier(const Messier&); // do not implement
				void operator=(const Messier&); // do not implement

				int create_pinchild(char* prog_binary, char** arg_list){return 0;}

				SST::Link * m_memChan; 

				SST::Link * event_link; // Note that this is a self-link for events

				NVM_PARAMS * nvm_params;
				NVM_DIMM * DIMM;

			
				long long int max_inst;
				char* named_pipe;
				int* pipe_id;
				std::string user_binary;
				Output* output;


				Statistic<long long int>* statReadRequests;
				Statistic<long long int>* statWriteRequests;
				Statistic<long long int>* statAvgTime;



		};

	}
}
