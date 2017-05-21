// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
#include <sst/core/elementinfo.h>
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

        // Internal Counters
        float s_link_phy_power;
        float s_link_local_route_power;
        float s_link_remote_route_power;
        float s_xbar_rqst_slot_power;
        float s_xbar_rsp_slot_power;
        float s_xbar_route_extern_power;
        float s_vault_rqst_slot_power;
        float s_vault_rsp_slot_power;
        float s_vault_ctrl_power;
        float s_row_access_power;
        float s_link_phy_therm;
        float s_link_local_route_therm;
        float s_link_remote_route_therm;
        float s_xbar_rqst_slot_therm;
        float s_xbar_rsp_slot_therm;
        float s_xbar_route_extern_therm;
        float s_vault_rqst_slot_therm;
        float s_vault_rsp_slot_therm;
        float s_vault_ctrl_therm;
        float s_row_access_therm;

        // Statistics
        Statistic<uint64_t>* Write16Ops;
        Statistic<uint64_t>* Write32Ops;
        Statistic<uint64_t>* Write48Ops;
        Statistic<uint64_t>* Write64Ops;
        Statistic<uint64_t>* Write80Ops;
        Statistic<uint64_t>* Write96Ops;
        Statistic<uint64_t>* Write112Ops;
        Statistic<uint64_t>* Write128Ops;
        Statistic<uint64_t>* Write256Ops;
        Statistic<uint64_t>* Read16Ops;
        Statistic<uint64_t>* Read32Ops;
        Statistic<uint64_t>* Read48Ops;
        Statistic<uint64_t>* Read64Ops;
        Statistic<uint64_t>* Read80Ops;
        Statistic<uint64_t>* Read96Ops;
        Statistic<uint64_t>* Read112Ops;
        Statistic<uint64_t>* Read128Ops;
        Statistic<uint64_t>* Read256Ops;
        Statistic<uint64_t>* ModeWriteOps;
        Statistic<uint64_t>* ModeReadOps;
        Statistic<uint64_t>* BWROps;
        Statistic<uint64_t>* TwoAdd8Ops;
        Statistic<uint64_t>* Add16Ops;
        Statistic<uint64_t>* PWrite16Ops;
        Statistic<uint64_t>* PWrite32Ops;
        Statistic<uint64_t>* PWrite48Ops;
        Statistic<uint64_t>* PWrite64Ops;
        Statistic<uint64_t>* PWrite80Ops;
        Statistic<uint64_t>* PWrite96Ops;
        Statistic<uint64_t>* PWrite112Ops;
        Statistic<uint64_t>* PWrite128Ops;
        Statistic<uint64_t>* PWrite256Ops;
        Statistic<uint64_t>* PBWROps;
        Statistic<uint64_t>* P2ADD8Ops;
        Statistic<uint64_t>* P2ADD16Ops;
        Statistic<uint64_t>* TwoAddS8ROps;
        Statistic<uint64_t>* AddS16ROps;
        Statistic<uint64_t>* Inc8Ops;
        Statistic<uint64_t>* PInc8Ops;
        Statistic<uint64_t>* Xor16Ops;
        Statistic<uint64_t>* Or16Ops;
        Statistic<uint64_t>* Nor16Ops;
        Statistic<uint64_t>* And16Ops;
        Statistic<uint64_t>* Nand16Ops;
        Statistic<uint64_t>* CasGT8Ops;
        Statistic<uint64_t>* CasGT16Ops;
        Statistic<uint64_t>* CasLT8Ops;
        Statistic<uint64_t>* CasLT16Ops;
        Statistic<uint64_t>* CasEQ8Ops;
        Statistic<uint64_t>* CasZero16Ops;
        Statistic<uint64_t>* Eq8Ops;
        Statistic<uint64_t>* Eq16Ops;
        Statistic<uint64_t>* BWR8ROps;
        Statistic<uint64_t>* Swap16Ops;

        // stall stats
        Statistic<uint64_t>* xbar_rqst_stall_stat;
        Statistic<uint64_t>* xbar_rsp_stall_stat;
        Statistic<uint64_t>* vault_rqst_stall_stat;
        Statistic<uint64_t>* vault_rsp_stall_stat;
        Statistic<uint64_t>* route_rqst_stall_stat;
        Statistic<uint64_t>* route_rsp_stall_stat;
        Statistic<uint64_t>* undef_stall_stat;
        Statistic<uint64_t>* bank_conflict_stat;
        Statistic<uint64_t>* xbar_latency_stat;

        // power & thermal stats
        Statistic<float>* link_phy_power_stat;
        Statistic<float>* link_local_route_power_stat;
        Statistic<float>* link_remote_route_power_stat;
        Statistic<float>* xbar_rqst_slot_power_stat;
        Statistic<float>* xbar_rsp_slot_power_stat;
        Statistic<float>* xbar_route_extern_power_stat;
        Statistic<float>* vault_rqst_slot_power_stat;
        Statistic<float>* vault_rsp_slot_power_stat;
        Statistic<float>* vault_ctrl_power_stat;
        Statistic<float>* row_access_power_stat;
        Statistic<float>* link_phy_therm_stat;
        Statistic<float>* link_local_route_therm_stat;
        Statistic<float>* link_remote_route_therm_stat;
        Statistic<float>* xbar_rqst_slot_therm_stat;
        Statistic<float>* xbar_rsp_slot_therm_stat;
        Statistic<float>* xbar_route_extern_therm_stat;
        Statistic<float>* vault_rqst_slot_therm_stat;
        Statistic<float>* vault_rsp_slot_therm_stat;
        Statistic<float>* vault_ctrl_therm_stat;
        Statistic<float>* row_access_therm_stat;

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

        void collectStats();
        void registerStatistics();
        void recordIOStats(uint64_t header);
	void zeroPacket(uint64_t* packet) const;
	void processResponses();
	void printPendingRequests();

};

}
}

#endif
