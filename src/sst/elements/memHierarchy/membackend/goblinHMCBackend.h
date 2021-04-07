// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include <vector>

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/output.h>
#include "sst/elements/memHierarchy/membackend/memBackend.h"

#include <list>

#include <hmc_sim.h>

namespace SST {
namespace MemHierarchy {

typedef struct{
  std::string name;
  hmc_rqst_t type;
  int data_len;
  int rqst_len;
  int rsp_len;
  bool isCMC;
} HMCPacket;

typedef enum{
  SRC_WR,
  SRC_RD,
  SRC_POSTED,
  SRC_CUSTOM,
}CMCSrcReq;

class HMCCMCConfig{
public:
  HMCCMCConfig( std::string P, hmc_rqst_t R, int RQ, int RS ) :
    path(P), cmd(R), rqst_flits(RQ), rsp_flits(RS) {}
  ~HMCCMCConfig();

  std::string getPath() { return path; }
  hmc_rqst_t getCmdType() { return cmd; }
  int getRqstFlits() { return rqst_flits; }
  int getRspFlits() { return rsp_flits; }

private:
  std::string path;
  hmc_rqst_t cmd;
  int rqst_flits;
  int rsp_flits;
};

class HMCSimCmdMap{
public:
  HMCSimCmdMap( CMCSrcReq SRC, uint32_t OPC, int SZ, hmc_rqst_t DEST ) :
    src(SRC), opc(OPC), size(SZ), rqst(DEST) {}
  ~HMCSimCmdMap() {}

  CMCSrcReq getSrcType() { return src; }
  int getSrcSize() { return size; }
  uint32_t getOpcode() { return opc; }
  hmc_rqst_t getTargetType() { return rqst; }

private:
  CMCSrcReq src;    // source request type
  int size;         // size of the request
  hmc_rqst_t rqst;  // target request type
  uint32_t opc;     // custom opcode
};

class HMCSimBackEndReq {
	public:
		HMCSimBackEndReq(MemBackend::ReqId r, Addr a, uint64_t sTime, bool hr) :
			req(r), addr(a), startTime(sTime), hasResp(hr) {}
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
        bool hasResponse() const {
                        return hasResp;
                }
	private:
        MemBackend::ReqId req;
        Addr addr;
	uint64_t startTime;
        bool hasResp;
};

class GOBLINHMCSimBackend : public ExtMemBackend {

public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(GOBLINHMCSimBackend, "memHierarchy", "goblinHMCSim", SST_ELI_ELEMENT_VERSION(1,0,0), "GOBLIN HMC Simulator driven memory timings", SST::MemHierarchy::ExtMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            { "verbose",	"Sets the verbosity of the backend output", "0" },
            { "device_count",	"Sets the number of HMCs being simulated, default=1, max=8", "1" },
	    { "link_count", 	"Sets the number of links being simulated, min=4, max=8, default=4", "4" },
	    { "vault_count",	"Sets the number of vaults being simulated, min=16, max=32, default=16", "16" },
	    { "queue_depth",	"Sets the depth of the HMC request queue, min=2, max=65536, default=2", "2" },
  	    { "dram_count",     "Sets the number of DRAM blocks per cube", "20" },
	    { "xbar_depth",     "Sets the queue depth for the HMC X-bar", "8" },
            { "max_req_size",   "Sets the maximum request size, in bytes", "64" },
#ifdef HMC_DEV_DRAM_LATENCY
            { "dram_latency",   "Sets the internal DRAM fetch latency in clock cycles", "2" },
#endif
	    { "trace-banks", 	"Decides where tracing for memory banks is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	    { "trace-queue", 	"Decides where tracing for queues is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	    { "trace-cmds", 	"Decides where tracing for commands is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	    { "trace-latency", 	"Decides where tracing for latency is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	    { "trace-stalls", 	"Decides where tracing for memory stalls is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
#ifdef HMC_TRACE_POWER
            { "trace-power",    "Decides where tracing for memory power is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
#endif
	    { "tag_count",	        "Sets the number of inflight tags that can be pending at any point in time", "16" },
	    { "capacity_per_device",    "Sets the capacity of the device being simulated in GiB, min=2, max=8, default is 4", "4" },
            { "cmc-config",             "Enables a CMC library command in HMCSim", "NONE" },
            { "cmd-map",                "Maps an existing HMC or CMC command to the target command type", "NONE" }  )

    SST_ELI_DOCUMENT_STATISTICS(
        {"WR16",            "Operation count for HMC WR16",       "count", 1},
        {"WR32",            "Operation count for HMC WR32",       "count", 1},
        {"WR48",            "Operation count for HMC WR48",       "count", 1},
        {"WR64",            "Operation count for HMC WR64",       "count", 1},
        {"WR80",            "Operation count for HMC WR80",       "count", 1},
        {"WR96",            "Operation count for HMC WR96",       "count", 1},
        {"WR112",           "Operation count for HMC WR112",      "count", 1},
        {"WR128",           "Operation count for HMC WR128",      "count", 1},
        {"WR256",           "Operation count for HMC WR256",      "count", 1},
        {"RD16",            "Operation count for HMC RD16",       "count", 1},
        {"RD32",            "Operation count for HMC RD32",       "count", 1},
        {"RD48",            "Operation count for HMC RD48",       "count", 1},
        {"RD64",            "Operation count for HMC RD64",       "count", 1},
        {"RD80",            "Operation count for HMC RD80",       "count", 1},
        {"RD96",            "Operation count for HMC RD96",       "count", 1},
        {"RD112",           "Operation count for HMC RD112",      "count", 1},
        {"RD128",           "Operation count for HMC RD128",      "count", 1},
        {"RD256",           "Operation count for HMC RD256",      "count", 1},
        {"MD_WR",           "Operation count for HMC MD_WR",      "count", 1},
        {"MD_RD",           "Operation count for HMC MD_RD",      "count", 1},
        {"BWR",             "Operation count for HMC BWR",        "count", 1},
        {"2ADD8",           "Operation count for HMC 2ADD8",      "count", 1},
        {"ADD16",           "Operation count for HMC ADD16",      "count", 1},
        {"P_WR16",          "Operation count for HMC P_WR16",     "count", 1},
        {"P_WR32",          "Operation count for HMC P_WR32",     "count", 1},
        {"P_WR48",          "Operation count for HMC P_WR48",     "count", 1},
        {"P_WR64",          "Operation count for HMC P_WR64",     "count", 1},
        {"P_WR80",          "Operation count for HMC P_WR80",     "count", 1},
        {"P_WR96",          "Operation count for HMC P_WR96",     "count", 1},
        {"P_WR112",         "Operation count for HMC P_WR112",    "count", 1},
        {"P_WR128",         "Operation count for HMC P_WR128",    "count", 1},
        {"P_WR256",         "Operation count for HMC P_WR256",    "count", 1},
        {"2ADDS8R",         "Operation count for HMC 2ADDS8R",    "count", 1},
        {"ADDS16",          "Operation count for HMC ADDS16",     "count", 1},
        {"INC8",            "Operation count for HMC INC8",       "count", 1},
        {"P_INC8",          "Operation count for HMC P_INC8",     "count", 1},
        {"XOR16",           "Operation count for HMC XOR16",      "count", 1},
        {"OR16",            "Operation count for HMC OR16",       "count", 1},
        {"NOR16",           "Operation count for HMC NOR16",      "count", 1},
        {"AND16",           "Operation count for HMC AND16",      "count", 1},
        {"NAND16",          "Operation count for HMC NAND16",     "count", 1},
        {"CASGT8",          "Operation count for HMC CASGT8",     "count", 1},
        {"CASGT16",         "Operation count for HMC CASGT16",    "count", 1},
        {"CASLT8",          "Operation count for HMC CASLT8",     "count", 1},
        {"CASLT16",         "Operation count for HMC CASLT16",    "count", 1},
        {"CASEQ8",          "Operation count for HMC CASEQ8",     "count", 1},
        {"CASZERO16",       "Operation count for HMC CASZER016",  "count", 1},
        {"EQ8",             "Operation count for HMC EQ8",        "count", 1},
        {"EQ16",            "Operation count for HMC EQ16",       "count", 1},
        {"BWR8R",           "Operation count for HMC BWR8R",      "count", 1},
        {"SWAP16",          "Operation count for HMC SWAP16",     "count", 1},
        {"XbarRqstStall",   "HMC Crossbar request stalls",        "count", 1},
        {"XbarRspStall",    "HMC Crossbar response stalls",       "count", 1},
        {"VaultRqstStall",  "HMC Vault request stalls",           "count", 1},
        {"VaultRspStall",   "HMC Vault response stalls",          "count", 1},
        {"RouteRqstStall",  "HMC Route request stalls",           "count", 1},
        {"RouteRspStall",   "HMC Route response stalls",          "count", 1},
        {"UndefStall",      "HMC Undefined stall events",         "count", 1},
        {"BankConflict",    "HMC Bank conflicts",                 "count", 1},
        {"XbarLatency",     "HMC Crossbar latency events",        "count", 1},

        {"LinkPhyPower",        "HMC Link phy power",                   "milliwatts", 1},
        {"LinkLocalRoutePower", "HMC Link local quadrant route power",  "milliwatts", 1},
        {"LinkRemoteRoutePower","HMC Link remote quadrante route power","milliwatts", 1},
        {"XbarRqstSlotPower",   "HMC Crossbar request slot power",      "milliwatts", 1},
        {"XbarRspSlotPower",    "HMC Crossbar response slot power",     "milliwatts", 1},
        {"XbarRouteExternPower","HMC Crossbar external route power",    "milliwatts", 1},
        {"VaultRqstSlotPower",  "HMC Vault request slot power",         "milliwatts", 1},
        {"VaultRspSlotPower",   "HMC Vault response slot power",        "milliwatts", 1},
        {"VaultCtrlPower",      "HMC Vault control power",              "milliwatts", 1},
        {"RowAccessPower",      "HMC DRAM row access power",            "milliwatts", 1},

        {"LinkPhyTherm",        "HMC Link phy thermal",                   "btus", 1},
        {"LinkLocalRouteTherm", "HMC Link local quadrant route thermal",  "btus", 1},
        {"LinkRemoteRouteTherm","HMC Link remote quadrante route thermal","btus", 1},
        {"XbarRqstSlotTherm",   "HMC Crossbar request slot thermal",      "btus", 1},
        {"XbarRspSlotTherm",    "HMC Crossbar response slot thermal",     "btus", 1},
        {"XbarRouteExternTherm","HMC Crossbar external route thermal",    "btus", 1},
        {"VaultRqstSlotTherm",  "HMC Vault request slot thermal",         "btus", 1},
        {"VaultRspSlotTherm",   "HMC Vault response slot thermal",        "btus", 1},
        {"VaultCtrlTherm",      "HMC Vault control thermal",              "btus", 1},
        {"RowAccessTherm",      "HMC DRAM row access thermal",            "btus", 1} )

/* Class definition */
        GOBLINHMCSimBackend(ComponentId_t id, Params& params);
	~GOBLINHMCSimBackend();
	bool issueRequest(ReqId, Addr, bool,
                          std::vector<uint64_t>,
                          uint32_t, unsigned);
	bool issueCustomRequest(ReqId, Addr, uint32_t,
                                std::vector<uint64_t>,
                                uint32_t, unsigned);
	void setup();
	void finish();
	virtual bool clock(Cycle_t cycle);

private:
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
        std::vector<std::string> cmcconfigs;
        std::vector<std::string> cmdmaps;

        std::list<HMCSimCmdMap *> CmdMapping;
        std::list<HMCCMCConfig *> CmcConfig;

	std::string hmc_trace_file;
	FILE* hmc_trace_file_handle;

	// We have to create a packet up to the maximum the sim will allow
	uint64_t hmc_packet[HMC_MAX_UQ_PACKET];

	// We are allowed up to 256-bytes in a payload but we may use less
        // now supports HMCC Spec 2.1
	uint64_t hmc_payload[32];

	std::queue<uint16_t> tag_queue;
	std::map<uint16_t, HMCSimBackEndReq*> tag_req_map;

        void handleCMCConfig();
        void handleCmdMap();

        void splitStr(const string& s,
                      char delim,
                      vector<string>& v);

        bool strToHMCRqst( std::string, hmc_rqst_t *, bool );
        bool HMCRqstToStr( hmc_rqst_t R, std::string *S );
        bool isPostedRqst( hmc_rqst_t );
        bool isReadRqst( hmc_rqst_t );

	bool issueMappedRequest(ReqId, Addr, bool,
                                std::vector<uint64_t>,
                                uint32_t, unsigned,
                                uint8_t, uint16_t, bool*);

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
