
#ifndef _H_SST_MIRANDA_REV_SINGLE_STREAM_GEN
#define _H_SST_MIRANDA_REV_SINGLE_STREAM_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class ReverseSingleStreamGenerator : public RequestGenerator {

public:
	ReverseSingleStreamGenerator( Component* owner, Params& params );
	~ReverseSingleStreamGenerator();
	void generate(std::queue<RequestGeneratorRequest*>* q);
	bool isFinished();
	void completed();

private:
	uint64_t startAddr;
	uint64_t stopAddr;
	uint64_t datawidth;
	uint64_t nextAddr;

	Output*  out;

};

}
}

#endif
