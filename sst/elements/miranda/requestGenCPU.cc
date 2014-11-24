
#include <sst_config.h>
#include "requestGenModule.h"
#include "requestGenCPU.h"

RequestGenCPU::RequestGenCPU(SST::ComponentId_t id, SST::Params& params) {
	const int verbose = (int) params.find_integer("verbose", 0);
	out = new Output();

	maxRequestsPending = (uint32_t) params.find_integer("maxmemoryreqs", 16);
	requestsPending = 0;

	out->verbose(CALL_INFO, 1, 0, "Configured CPU to allow %" PRIu32 " maximum requests to be memory to be outstanding.\n",
		maxRequestsPending);

	std::string reqGenModName = params.find_string("generator", "");

	out->verbose(CALL_INFO, 1, 0, "Request generator to be loaded is: %s\n", reqGenModName.c_str());
	Params genParams = params.find_prefix_params("generatorParams.");
	reqGen = dynamic_cast<RequestGenerator*>( loadModuleWithComponent(reqGenModName, this, genParams) );

	if(NULL == reqGen) {
		out->fatal(CALL_INFO, -1, "Failed to load generator: %s\n", reqGenModName.c_str());
	}
}

RequestGenCPU::~RequestGenCPU() {

}

void RequestGenCPU::finish() {

}

void RequestGenCPU::init(unsigned int phase) {

}

void RequestGenCPU::handleEvent( Interfaces::SimpleMem::Request* ev) {

}

bool RequestGenCPU::clockTick(SST::Cycle_t cycle) {
	return false;
}
