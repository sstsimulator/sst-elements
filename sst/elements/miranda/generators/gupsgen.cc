
#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/elements/miranda/generators/gupsgen.h>

using namespace SST::Miranda;

GUPSGenerator::GUPSGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);

	out = new Output("GUPSGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	issueCount = (uint64_t) params.find_integer("count", 1000);
	reqLength  = (uint64_t) params.find_integer("length", 8);
	maxAddr    = (uint64_t) params.find_integer("max_address", 524288);

	rng = new MarsagliaRNG(11, 31);

	out->verbose(CALL_INFO, 1, 0, "Will issue %" PRIu64 " operations\n", issueCount);
	out->verbose(CALL_INFO, 1, 0, "Request lengths: %" PRIu64 " bytes\n", reqLength);
	out->verbose(CALL_INFO, 1, 0, "Maximum address: %" PRIu64 "\n", maxAddr);
}

GUPSGenerator::~GUPSGenerator() {
	delete out;
	delete rng;
}

void GUPSGenerator::generate(MirandaRequestQueue* q) {
	out->verbose(CALL_INFO, 4, 0, "Generating next request number: %" PRIu64 "\n", issueCount);

	const uint64_t rand_addr = rng->generateNextUInt64();
	// Ensure we have a reqLength aligned request
	const uint64_t addr_under_limit = (rand_addr % maxAddr);
	const uint64_t addr = (addr_under_limit < reqLength) ? addr_under_limit :
		(rand_addr % maxAddr) - (rand_addr % reqLength);

	q->push(new MemoryOpRequest(addr, reqLength, READ));
	q->push(new MemoryOpRequest(addr, reqLength, WRITE));

	issueCount--;
}

bool GUPSGenerator::isFinished() {
	return (issueCount == 0);
}

void GUPSGenerator::completed() {

}
