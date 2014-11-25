
#ifndef _H_SST_MEM_H_REQUEST_GEN
#define _H_SST_MEM_H_REQUEST_GEN

#include <sst/core/module.h>

namespace SST {
namespace MemHierarchy {

typedef enum {
	READ,
	WRITE
} RequestGenOperation;

class RequestGeneratorRequest {
public:
	RequestGenOp(const uint64_t addr,
		const uint64_t length,
		RequestGenOperation opType) : physAddr(addr),
			len(length), op(opType) {};
	~RequestGenOp();
	uint64_t getAddress() const { return physAddr; }
	uint64_t getLength() const { return length; }
	RequestGenOperation getOperation() const { return op; }
protected:
	uint64_t physAddr;
	uint64_t len;
	RequestGenOperation op;
};

class RequestGenerator : public Module {

public:
	RequestGenerator( Component* owner, Params& params ) {};
	~RequestGenerator() {};
	virtual RequestGeneratorRequest* nextRequest() { return new RequestGeneratorRequest(0, READ); };
	virtual bool isFinished() { return true; }

};

}
}

#endif
