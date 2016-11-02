// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */



#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleMem.h>

#include <sst/core/output.h>

#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>

#include "TLBhierarchy.h"

//#include "arielcore.h"

using namespace std;

namespace SST {
	namespace SambaComponent {

		class Samba : public SST::Component {
			public:

				Samba(SST::ComponentId_t id, SST::Params& params); 
				void setup()  { };
				void finish() {for(int i=0; i<(int) core_count; i++) TLB[i]->finish();};
				void handleEvent(SST::Event* event) {};
				bool tick(SST::Cycle_t x);

			private:
				Samba();  // for serialization only
				Samba(const Samba&); // do not implement
				void operator=(const Samba&); // do not implement

				int create_pinchild(char* prog_binary, char** arg_list){return 0;}


				SST::Link ** cpu_to_mmu;

				TLBhierarchy ** TLB;

				SST::Link ** mmu_to_cache;

				long long int max_inst;
				char* named_pipe;
				int* pipe_id;
				std::string user_binary;
				Output* output;

				TLBhierarchy ** cores;
				uint32_t core_count;
				SST::Link** Samba_link;


				Statistic<long long int>* statReadRequests;


		};

	}
}
