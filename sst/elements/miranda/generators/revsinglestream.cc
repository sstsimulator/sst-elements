
#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/revsinglestream.h>

using namespace SST::Miranda;

ReverseSingleStreamGenerator::ReverseSingleStreamGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);

	out = new Output("ReverseSingleStreamGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	stopAddr   = (uint64_t) params.find_integer("stop_address", 0);
	startAddr  = (uint64_t) params.find_integer("start_address", 1024);
	datawidth  = (uint64_t) params.find_integer("datawidth", 8);

	// Ensure we really are going backwards
	assert(startAddr < stopAddr);

	nextAddr = startAddr;
}

ReverseSingleStreamGenerator::~ReverseSingleStreamGenerator() {
	delete out;
}

void ReverseSingleStreamGenerator::generate(std::queue<RequestGeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 4, 0, "Generating next request at address: %" PRIu64 "\n", nextAddr);

	RequestGeneratorRequest* nextReq = new RequestGeneratorRequest();
	nextReq->set(nextAddr * datawidth, datawidth, READ);

	q->push(nextReq);

	// What is the next address?
	nextAddr--;
}

bool ReverseSingleStreamGenerator::isFinished() {
	return (nextAddr == stopAddr);
}

void ReverseSingleStreamGenerator::completed() {

}
