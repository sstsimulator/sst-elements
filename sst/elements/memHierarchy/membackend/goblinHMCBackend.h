
#ifndef _H_SST_MEMH_GOBLIN_HMC_BACKEND
#define _H_SST_MEMH_GOBLIN_HMC_BACKEND

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/output.h>
#include <membackend/memBackend.h>

#include <hmc_sim.h>

namespace SST {
namespace MemHierarchy {

class GOBLINHMCSimBackend : public MemBackend {

public:
	GOBLINHMCSimBackend() {};MemController::DRAMReq* req)
	GOBLINHMCSimBackend(Component* comp, Params& params);
	~GOBLINHMCSimBackend();
	bool issueRequest(MemController::DRAMReq* req);
	void setup();
	void finish();
	void clock();

private:
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

	// We have to create a packet up to the maximum the sim will allow
	uint64_t hmc_packet[HMC_MAX_UQ_PACKET];
	// We are allowed up to 128-bytes in a payload but we may use less
	uint64_t hmc_payload[16];

	std::queue<uint16_t> tag_queue;
	std::map<uint16_t, MemController::DRAMReq*> tag_req_map;

	void zeroPacket(uint64_t* packet) const;
	void processResponses();

};

}
}

#endif
