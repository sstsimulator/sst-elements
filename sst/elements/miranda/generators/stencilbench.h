
#ifndef _H_SST_MIRANDA_STENCIL_BENCH_GEN
#define _H_SST_MIRANDA_STENCIL_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class StencilBenchGenerator : public RequestGenerator {

public:
	StencilBenchGenerator( Component* owner, Params& params );
	~StencilBenchGenerator();
	void generate(std::queue<RequestGeneratorRequest*>* q);
	bool isFinished();
	void completed();

private:
	void convertIndexToPosition(const uint32_t index,
		uint32_t* posX, uint32_t* posY, uint32_t* posZ);
	void convertPositionToIndex(const uint32_t posX,
		const uint32_t posY, const uint32_t posZ, uint32_t* index);

	uint32_t nX;
	uint32_t nY;
	uint32_t nZ;
	uint32_t datawidth;

	uint32_t startX;
	uint32_t endX;

	Output*  out;

};

}
}

#endif
