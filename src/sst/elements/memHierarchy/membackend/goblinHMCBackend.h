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

#include <queue>

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
		HMCSimBackEndReq(MemBackend::ReqId r, Addr a, uint64_t sTime) :
			req(r), addr(a), startTime(sTime) {}
		~HMCSimBackEndReq() {}

		uint64_t getStartTime() const {
			return startTime;
		}

        MemBackend::ReqId getRequest() const {
			return req;
		}
        Addr getAddr() const {
			return addr;
		}
	private:
        MemBackend::ReqId req;
        Addr addr;
		uint64_t startTime;
};

class GOBLINHMCSimBackend : public SimpleMemBackend {

public:
	GOBLINHMCSimBackend() : SimpleMemBackend() {};
	GOBLINHMCSimBackend(Component* comp, Params& params);
	~GOBLINHMCSimBackend();
	bool issueRequest(ReqId, Addr, bool, unsigned);
	void setup();
	void finish();
	void clock();

private:
	Component* owner;
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
        uint32_t hmc_dram_latency;

        float link_phy;
        float link_local_route;
        float link_remote_route;
        float xbar_rqst_slot;
        float xbar_rsp_slot;
        float xbar_route_extern;
        float vault_rqst_slot;
        float vault_rsp_slot;
        float vault_ctrl;
        float row_access;

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
