
#ifndef _H_SST_MEM_H_REQUEST_GEN
#define _H_SST_MEM_H_REQUEST_GEN

#include <stdint.h>
#include <sst/core/module.h>
#include <sst/core/component.h>

namespace SST {
namespace Miranda {

typedef enum {
	READ,
	WRITE
} RequestGenOperation;

class RequestGeneratorRequest {
public:
	RequestGeneratorRequest(const uint64_t addr,
		const uint64_t length,
		RequestGenOperation opType) : physAddr(addr),
			len(length), op(opType) {};
	~RequestGeneratorRequest() { }
	uint64_t getAddress() const { return physAddr; }
	uint64_t getLength() const { return len; }
	RequestGenOperation getOperation() const { return op; }
	bool     isRead() const { return op == READ; }
	bool     isWrite() const { return op == WRITE; }
protected:
	uint64_t physAddr;
	uint64_t len;
	RequestGenOperation op;
};

class RequestGenerator : public Module {

public:
	RequestGenerator( Component* owner, Params& params ) {};
	~RequestGenerator() {};
	virtual RequestGeneratorRequest* nextRequest() { return new RequestGeneratorRequest(0, 0, READ); };
	virtual bool isFinished() { return true; }

};

}
}

#endif
