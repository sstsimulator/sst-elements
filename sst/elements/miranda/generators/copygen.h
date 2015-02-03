
#ifndef _H_SST_MIRANDA_COPY_BENCH_GEN
#define _H_SST_MIRANDA_COPY_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>
#include <sst/core/rng/sstrand.h>

#include <queue>

using namespace SST::RNG;

namespace SST {
namespace Miranda {

class CopyGenerator : public RequestGenerator {

public:
	CopyGenerator( Component* owner, Params& params ) : RequestGenerator(owner, params) {
		const uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);
		out = new Output("CopyGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

		readAddr  = (uint64_t) params.find_integer("read_start_address",  0);
		reqLength = (uint64_t) params.find_integer("request_size", 8);
		itemCount = (uint64_t) params.find_integer("request_count", 1024);

		// Write address default is sized for number of requests * req lengtgh
		writeAddr = (uint64_t) params.find_integer("write_start_address",
			readAddr + (reqLength * itemCount));

		// Start generator at 0
		nextItem = 0;
	}

	~CopyGenerator() {
		delete out;
	}

	void generate(MirandaRequestQueue<GeneratorRequest*>* q) {
		if(nextItem == itemCount) {
			return;
		}

		q->push_back(new MemoryOpRequest(readAddr  + (nextItem * reqLength), reqLength, READ)  );
		q->push_back(new MemoryOpRequest(writeAddr + (nextItem * reqLength), reqLength, WRITE) );

		nextItem++;
	}

	bool isFinished() {
		return (nextItem == itemCount);
	}

	void completed() {}

private:
	uint64_t nextItem;
	uint64_t readAddr;
	uint64_t writeAddr;
	uint64_t itemCount;
	uint64_t reqLength;
	Output*  out;

};

}
}

#endif
