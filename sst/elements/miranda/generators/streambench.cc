
#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/streambench.h>

using namespace SST::Miranda;

STREAMBenchGenerator::STREAMBenchGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);

	out = new Output("STREAMBenchGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	n = (uint64_t) params.find_integer("n", 10000);
	reqLength = (uint64_t) params.find_integer("length", 8);

	start_a = 0;
	start_b = (n * reqLength);
	start_c = 2 * (n * reqLength);

	i = 0;

	out->output("STREAM-N length is %" PRIu64 "\n", n);
	out->output("Start of array a @ %" PRIu64 "\n", start_a);
	out->output("Start of array b @ %" PRIu64 "\n", start_b);
	out->output("Start of array c @ %" PRIu64 "\n", start_c);
	out->output("Array Length:      %" PRIu64 " bytes\n", (n * reqLength));
	out->output("Total arrays:      %" PRIu64 " bytes\n", (3 * n * reqLength));
}

STREAMBenchGenerator::~STREAMBenchGenerator() {
	delete out;
}

void STREAMBenchGenerator::generate(std::queue<RequestGeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 4, 0, "Array index: %" PRIu64 "\n", i);

	RequestGeneratorRequest* read_b = new RequestGeneratorRequest();
	read_b->set(start_b + (i * reqLength), reqLength, READ);
	q->push(read_b);

	RequestGeneratorRequest* read_c = new RequestGeneratorRequest();
	read_c->set(start_c + (i * reqLength), reqLength, READ);
	q->push(read_c);

	RequestGeneratorRequest* write_a = new RequestGeneratorRequest();
	write_a->set(start_a + (i * reqLength), reqLength, WRITE);
	q->push(write_a);

	i++;
}

bool STREAMBenchGenerator::isFinished() {
	return (i == n);
}

void STREAMBenchGenerator::completed() {

}
