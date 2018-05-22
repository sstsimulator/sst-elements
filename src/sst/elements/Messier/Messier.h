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
