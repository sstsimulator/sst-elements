
#ifndef _H_SST_MIRANDA_RANDOM_GEN
#define _H_SST_MIRANDA_RANDOM_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>
#include <sst/core/rng/sstrand.h>

#include <queue>

using namespace SST::RNG;

namespace SST {
namespace Miranda {

class RandomGenerator : public RequestGenerator {

public:
	RandomGenerator( Component* owner, Params& params );
	~RandomGenerator();
	void generate(std::queue<RequestGeneratorRequest*>* q);
	bool isFinished();
	void completed();

private:
	uint64_t reqLength;
	uint64_t start;
	uint64_t maxAddr;
	uint64_t issueCount;
	SSTRandom* rng;
	Output*  out;

};

}
}

#endif
