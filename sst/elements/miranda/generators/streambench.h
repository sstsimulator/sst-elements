
#ifndef _H_SST_MIRANDA_STREAM_BENCH_GEN
#define _H_SST_MIRANDA_STREAM_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class STREAMBenchGenerator : public RequestGenerator {

public:
	STREAMBenchGenerator( Component* owner, Params& params );
	~STREAMBenchGenerator();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

private:
	uint64_t reqLength;

	uint64_t start_a;
	uint64_t start_b;
	uint64_t start_c;

	uint64_t n;
	uint64_t n_per_call;
	uint64_t i;

	Output*  out;

};

}
}

#endif
