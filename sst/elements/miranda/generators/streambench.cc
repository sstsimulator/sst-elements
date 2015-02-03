
#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/streambench.h>

using namespace SST::Miranda;

STREAMBenchGenerator::STREAMBenchGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);

	out = new Output("STREAMBenchGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	n = (uint64_t) params.find_integer("n", 10000);
	reqLength = (uint64_t) params.find_integer("operandwidth", 8);

	start_a = (uint64_t) params.find_integer("start_a", 0);
	const uint64_t def_b = start_a + (n * reqLength);

	start_b = (uint64_t) params.find_integer("start_b", def_b);

	const uint64_t def_c = start_b + (2 * (n * reqLength));
	start_c = (uint64_t) params.find_integer("start_c", def_c);

	n_per_call = (uint64_t) params.find_integer("n_per_call", 1);

	i = 0;

	out->verbose(CALL_INFO, 1, 0, "STREAM-N length is %" PRIu64 "\n", n);
	out->verbose(CALL_INFO, 1, 0, "Start of array a @ %" PRIu64 "\n", start_a);
	out->verbose(CALL_INFO, 1, 0, "Start of array b @ %" PRIu64 "\n", start_b);
	out->verbose(CALL_INFO, 1, 0, "Start of array c @ %" PRIu64 "\n", start_c);
	out->verbose(CALL_INFO, 1, 0, "Array Length:      %" PRIu64 " bytes\n", (n * reqLength));
	out->verbose(CALL_INFO, 1, 0, "Total arrays:      %" PRIu64 " bytes\n", (3 * n * reqLength));
	out->verbose(CALL_INFO, 1, 0, "N-per-generate     %" PRIu64 "\n", n_per_call);
}

STREAMBenchGenerator::~STREAMBenchGenerator() {
	delete out;
}

void STREAMBenchGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q) {
	for(uint64_t j = 0; j < n_per_call; ++j) {
		out->verbose(CALL_INFO, 4, 0, "Array index: %" PRIu64 "\n", i);

		// If we reached our limit then step out of the generation
		if(i == n) {
			break;
		}

		MemoryOpRequest* read_b  = new MemoryOpRequest(start_b + (i * reqLength), reqLength, READ);
		MemoryOpRequest* read_c  = new MemoryOpRequest(start_c + (i * reqLength), reqLength, READ);
		MemoryOpRequest* write_a = new MemoryOpRequest(start_a + (i * reqLength), reqLength, WRITE);

		write_a->addDependency(read_b->getRequestID());
		write_a->addDependency(read_c->getRequestID());

		out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", (start_b + (i * reqLength)));
		q->push_back(read_b);

		out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", (start_c + (i * reqLength)));
		q->push_back(read_c);

		out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", (start_a + (i * reqLength)));
		q->push_back(write_a);

		i++;
	}
}

bool STREAMBenchGenerator::isFinished() {
	return (i == n);
}

void STREAMBenchGenerator::completed() {

}
