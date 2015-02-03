
#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/revsinglestream.h>

using namespace SST::Miranda;

ReverseSingleStreamGenerator::ReverseSingleStreamGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);

	out = new Output("ReverseSingleStreamGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	stopIndex   = (uint64_t) params.find_integer("stop_at", 0);
	startIndex  = (uint64_t) params.find_integer("start_at", 1024);
	datawidth   = (uint64_t) params.find_integer("datawidth", 8);
	stride      = (uint64_t) params.find_integer("stride", 1);

	if(startIndex < stopIndex) {
		out->fatal(CALL_INFO, -1, "Start address (%" PRIu64 ") must be greater than stop address (%" PRIu64 ") in reverse stream generator",
			startIndex, stopIndex);
	}

	out->verbose(CALL_INFO, 1, 0, "Start Address:         %" PRIu64 "\n", startIndex);
	out->verbose(CALL_INFO, 1, 0, "Stop Address:          %" PRIu64 "\n", stopIndex);
	out->verbose(CALL_INFO, 1, 0, "Data width:            %" PRIu64 "\n", datawidth);
	out->verbose(CALL_INFO, 1, 0, "Stride:                %" PRIu64 "\n", stride);

	nextIndex = startIndex;
}

ReverseSingleStreamGenerator::~ReverseSingleStreamGenerator() {
	delete out;
}

void ReverseSingleStreamGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 4, 0, "Generating next request at address: %" PRIu64 "\n", nextIndex);

	q->push_back(new MemoryOpRequest(nextIndex * datawidth, datawidth, READ));

	// What is the next address?
	nextIndex = nextIndex - stride;
}

bool ReverseSingleStreamGenerator::isFinished() {
	return (nextIndex == stopIndex);
}

void ReverseSingleStreamGenerator::completed() {

}
