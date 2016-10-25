// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_GOBLIN_HMC_BACKEND
#define _H_SST_MEMH_GOBLIN_HMC_BACKEND

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/output.h>
#include <membackend/memBackend.h>

#include <list>

#include <hmc_sim.h>

namespace SST {
namespace MemHierarchy {

class HMCSimBackEndReq {
	public:
		HMCSimBackEndReq(DRAMReq* r, uint64_t sTime) :
			req(r), startTime(sTime) {}
		~HMCSimBackEndReq() {}

		uint64_t getStartTime() const {
			return startTime;
		}

		DRAMReq* getRequest() const {
			return req;
		}
	private:
		DRAMReq* req;
		uint64_t startTime;
};

class GOBLINHMCSimBackend : public MemBackend {

public:
	GOBLINHMCSimBackend() : MemBackend() {};
	GOBLINHMCSimBackend(Component* comp, Params& params);
	~GOBLINHMCSimBackend();
	bool issueRequest(DRAMReq* req);
	void setup();
	void finish();
	void clock();

private:
	Component* owner;
	Output* output;
	struct hmcsim_t the_hmc;

	uint32_t hmc_link_count;
	uint32_t hmc_dev_count;
	uint32_t hmc_vault_count;
	uint32_t hmc_queue_depth;
	uint32_t hmc_bank_count;
	uint32_t hmc_dram_count;
	uint32_t hmc_capacity_per_device;
	uint32_t hmc_xbar_depth;
	uint32_t hmc_max_req_size;
	uint32_t hmc_max_trace_level;
	uint32_t hmc_tag_count;
	uint32_t hmc_trace_level;

	uint32_t nextLink;

        std::vector<std::string> cmclibs;

	std::string hmc_trace_file;
	FILE* hmc_trace_file_handle;

	// We have to create a packet up to the maximum the sim will allow
	uint64_t hmc_packet[HMC_MAX_UQ_PACKET];
	// We are allowed up to 128-bytes in a payload but we may use less
	uint64_t hmc_payload[16];

	std::queue<uint16_t> tag_queue;
	std::map<uint16_t, HMCSimBackEndReq*> tag_req_map;

	void zeroPacket(uint64_t* packet) const;
	void processResponses();
	void printPendingRequests();

};

}
}

#endif
