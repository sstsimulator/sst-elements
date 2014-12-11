
#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/singlestream.h>

using namespace SST::Miranda;

SingleStreamGenerator::SingleStreamGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);

	out = new Output("SingleStreamGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	issueCount = (uint64_t) params.find_integer("count", 1000);
	reqLength  = (uint64_t) params.find_integer("length", 8);
	nextAddr   = (uint64_t) params.find_integer("startat", 0);
	maxAddr    = (uint64_t) params.find_integer("max_address", 524288);

	out->verbose(CALL_INFO, 1, 0, "Will issue %" PRIu64 " operations\n", issueCount);
	out->verbose(CALL_INFO, 1, 0, "Request lengths: %" PRIu64 " bytes\n", reqLength);
	out->verbose(CALL_INFO, 1, 0, "Maximum address: %" PRIu64 "\n", maxAddr);
	out->verbose(CALL_INFO, 1, 0, "First address: %" PRIu64 "\n", nextAddr);
}

SingleStreamGenerator::~SingleStreamGenerator() {
	delete out;
}

void SingleStreamGenerator::generate(std::queue<RequestGeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 4, 0, "Generating next request number: %" PRIu64 "\n", issueCount);

	RequestGeneratorRequest* nextReq = new RequestGeneratorRequest();
	nextReq->set(nextAddr, reqLength, READ);

	q->push(nextReq);

	// What is the next address?
	nextAddr = (nextAddr + reqLength) % maxAddr;

	issueCount--;
}

bool SingleStreamGenerator::isFinished() {
	return (issueCount == 0);
}

void SingleStreamGenerator::completed() {

}
