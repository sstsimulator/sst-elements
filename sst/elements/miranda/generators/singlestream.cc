
#include <sst_config.h>
#include <sst/elements/miranda/generators/singlestream.h>

SingleStreamGenerator::SingleStreamGenerator( Component* owner, Params& params ) {
	issueCount = (uint64_t) params.find_integer("count", 1000);
	reqLength  = (uint64_t) params.find_integer("length", 8);
	nextAddr   = (uint64_t) params.find_integer("startat", 0);
	maxAddr    = (uint64_t) params.find_integer("max_address", 8192);
}

SingleStreamGenerator::~SingleStreamGenerator() {

}

RequestGeneratorRequest* SingleStreamGenerator::nextRequest() {
	RequestGeneratorRequest* nextReq = new RequestGeneratorRequest(
		nextAddr, reqLength, READ);

	nextAddr = (nextAddr + reqLength) % maxAddr;
	issueCount--;

	return nextReq;
}

bool SingleStreamGenerator::isFinished() {
	return (issueCount == 0);
}
