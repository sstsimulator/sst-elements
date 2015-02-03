
#ifndef _H_SST_MIRANDA_EMPTY_GEN
#define _H_SST_MIRANDA_EMPTY_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>
#include <sst/core/rng/sstrand.h>

#include <queue>

using namespace SST::RNG;

namespace SST {
namespace Miranda {

class EmptyGenerator : public RequestGenerator {

public:
	EmptyGenerator( Component* owner, Params& params ) : RequestGenerator(owner, params) {}
	~EmptyGenerator() { }
	void generate(MirandaRequestQueue<GeneratorRequest*>* q) { }
	bool isFinished() { return true; }
	void completed() { }

};

}
}

#endif
