
#ifndef _H_SST_MIRANDA_SINGLE_STREAM_GEN
#define _H_SST_MIRANDA_SINGLE_STREAM_GEN

#include <sst/elements/miranda/reqGenModule.h>
#include <sst/core/output.h>

namespace SST {
namespace Miranda {

class SingleStreamGenerator : public RequestGenerator {

public:
	SingleStreamGenerator( Component* owner, Params& params );
	~SingleStreamGenerator();
	void nextRequest(RequestGeneratorRequest* req);
	bool isFinished();
	void completed();

private:
	uint64_t reqLength;
	uint64_t start;
	uint64_t maxAddr;
	uint64_t issueCount;
	uint64_t nextAddr;
	Output*  out;

};

}
}

#endif
