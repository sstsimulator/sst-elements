
#ifndef _H_SST_MEM_H_REQUEST_GEN
#define _H_SST_MEM_H_REQUEST_GEN

#include <stdint.h>
#include <sst/core/module.h>
#include <sst/core/component.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

typedef enum {
	READ,
	WRITE
} RequestGenOperation;

class RequestGeneratorRequest {
public:
	RequestGeneratorRequest() : physAddr(0), len(0), op(READ), issued(false) {}
	~RequestGeneratorRequest() { }
	uint64_t getAddress() const { return physAddr; }
	uint64_t getLength() const { return len; }
	RequestGenOperation getOperation() const { return op; }
	bool     isRead() const { return op == READ; }
	bool     isWrite() const { return op == WRITE; }
	bool	 isIssued() const { return issued; }
	void	 markIssued() { issued = true; }
	void     set(const uint64_t addr, const uint64_t length, const RequestGenOperation opType) {
		physAddr = addr;
		len = length;
		op = opType;
		issued = false;
	}
protected:
	uint64_t physAddr;
	uint64_t len;
	RequestGenOperation op;
	bool issued;
};

class RequestGenerator : public Module {

public:
	RequestGenerator( Component* owner, Params& params) {}
	~RequestGenerator() {}
	virtual void generate(std::queue<RequestGeneratorRequest*>* q) { }
	virtual bool isFinished() { return true; }
	virtual void completed() { }

};

}
}

#endif
