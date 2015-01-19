
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

	i = 0;

	out->verbose(CALL_INFO, 1, 0, "STREAM-N length is %" PRIu64 "\n", n);
	out->verbose(CALL_INFO, 1, 0, "Start of array a @ %" PRIu64 "\n", start_a);
	out->verbose(CALL_INFO, 1, 0, "Start of array b @ %" PRIu64 "\n", start_b);
	out->verbose(CALL_INFO, 1, 0, "Start of array c @ %" PRIu64 "\n", start_c);
	out->verbose(CALL_INFO, 1, 0, "Array Length:      %" PRIu64 " bytes\n", (n * reqLength));
	out->verbose(CALL_INFO, 1, 0, "Total arrays:      %" PRIu64 " bytes\n", (3 * n * reqLength));
}

STREAMBenchGenerator::~STREAMBenchGenerator() {
	delete out;
}

void STREAMBenchGenerator::generate(std::queue<RequestGeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 4, 0, "Array index: %" PRIu64 "\n", i);

	RequestGeneratorRequest* read_b = new RequestGeneratorRequest();
	out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", (start_b + (i * reqLength)));
	read_b->set(start_b + (i * reqLength), reqLength, READ);
	q->push(read_b);

	RequestGeneratorRequest* read_c = new RequestGeneratorRequest();
	out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", (start_c + (i * reqLength)));
	read_c->set(start_c + (i * reqLength), reqLength, READ);
	q->push(read_c);

	RequestGeneratorRequest* write_a = new RequestGeneratorRequest();
	out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", (start_a + (i * reqLength)));
	write_a->set(start_a + (i * reqLength), reqLength, WRITE);
	q->push(write_a);

	i++;
}

bool STREAMBenchGenerator::isFinished() {
	return (i == n);
}

void STREAMBenchGenerator::completed() {

}
