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


//
/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */

#ifndef _H_SST_NVM_DIMM
#define _H_SST_NVM_DIMM


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/core/link.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include<map>
#include<list>
#include "Rank.h"
#include "WriteBuffer.h"
#include "NVM_Params.h"
#include "NVM_Request.h"
#include "memReqEvent.h"
#include "Cache.h"

using namespace SST;
using namespace SST::MessierComponent;

namespace SST { namespace MessierComponent{

	// This class structure represents NVM-Based DIMM, including the NVM-DIMM controller
	class NVM_DIMM
	{ 

		bool enabled;

		// This determines the number of clock cycles so far
		long long int cycles;

		// The NVM parameters of this object
		NVM_PARAMS * params;

		// This is the requests buffer, where all transactions are buffered before being processed by the controller
		std::list<NVM_Request *> transactions;	

		// This tracks the currently outstanding requests
		std::list<NVM_Request *> outstanding;

		// This is used to quickly track the number of writes complete at a specific cycle to remove them from the currently executed writes
		std::map<long long int, int> WRITES_COMPLETE;
		
		// This is used to quickly track the number of reads complete at a specific cycle to remove them from the currently executed reads
		std::map<long long int, int> READS_COMPLETE;

		// This is the queue of the ready requests 
		std::map<NVM_Request *, long long int> ready_trans;

		// This tracks if a request is expected to be ready at the PCM
		std::map<NVM_Request *, long long int> ready_at_NVM;

		// This determines the completed requests and when they are completed
		std::list<NVM_Request *> completed_requests;
		
		// This holds a pointer to the ranks. Note: for NVM devices, most likely there will be no ranks concept, thus it will be only one rank
		RANK ** ranks;

		// This is a pointer for the write buffer
		NVM_WRITE_BUFFER * WB;

		// The number of currently executed reads
		int curr_reads;

		// The number of currently executed writes
		int curr_writes;

		//TODO: remove this variable
		int read_count;

		// This determines if cache line interleaving was used
		bool cacheline_interleave; 
		
		SST::Link * m_memChan;

		SST::Link * m_EventChan;

		std::map<long long int, MemReqEvent *> NVM_EVENT_MAP;

		std::map<NVM_Request *, long long int> TIME_STAMP;

		// This keeps track of the squashed requests, as they hit in the cache
		std::map<long long int, int> SQUASHED;

		// This structure prevents returning data before checking the cache, to avoid any inconsistency issues
		std::map<long long int, int> HOLD;

		// This keeps track of the owner object
		SST::Component * Owner;

		// This defines the internal cache of the NVM-based DIMM
		NVM_CACHE * cache;

		std::map<int, int> bank_hist;

		int group_locked;

		public: 

		// This is the constructor for the NVM-based DIMM
		NVM_DIMM(SST::Component * owner, NVM_PARAMS par); 

		// This is the clock of the near memory controller
		bool tick();
		
		void finish(){}

		RANK * getRank(long long int add){ return ranks[WhichRank(add)]; }
		BANK * getBank( long long int add) { return (ranks[WhichRank(add)])->getBank(WhichBank(add));}

		// SecondChance: This is for evaluating the idea of issuing requests to free banks
		BANK * getFreeBank( long long int add);

		// This determines the location of the block (in which rank), based on the interleaving policy
		int WhichRank(long long int add);

		// This determines the location of the block (in which bank), based on the interleaving policy
		int WhichBank(long long int add);

		//bool push_request(NVM_Request * req) { if(transactions.size() >= params->max_requests) return false; else {transactions.push_back(req); return true; }}
		
		bool push_request(NVM_Request * req) { transactions.push_back(req);  if(req->Read) TIME_STAMP[req]= cycles; return true;}

		// This is the optimized version that basiclly tries to find out if there is any possibility to achieve a row buffer hit from the current transactions
		bool submit_request_opt();

		// Check if it exists in the write buffer and delete it from their if exists
		bool find_in_wb(NVM_Request * temp);

		// This schedule a deliver for data ready at the NVM Chips
		void schedule_delivery();

		// Try to flush the write buffer
		bool try_flush_wb();

		// Try to find a row buffer hit and prioritize it over all other requests;
		bool pop_optimal();
	
		NVM_Request * pop_request();
		
		void setMemChannel(SST::Link * x) { m_memChan = x; }
		void setEventChannel(SST::Link * x) { m_EventChan = x; }

		void handleRequest(SST::Event* event);

		void handleEvent(SST::Event* event);
		// This is used to check if it is a row buffer hit or miss
		bool row_buffer_hit(long long int add, long long int bank_add);
		Statistic<uint64_t>* histogram_idle;

		Statistic<uint64_t>* reads;
		Statistic<uint64_t>* writes;

	};
}}

#endif
