
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
	WRITE,
	REQ_FENCE
} ReqOperation;

class GeneratorRequest {
public:
	GeneratorRequest() {}
	virtual ~GeneratorRequest() {}
	virtual ReqOperation getOperation() const = 0;
};

typedef std::queue<GeneratorRequest*> MirandaRequestQueue;

class MemoryOpRequest : public GeneratorRequest {
public:
	MemoryOpRequest(const uint64_t cAddr,
		const uint64_t cLength,
		const ReqOperation cOpType) :
		GeneratorRequest(),
		addr(cAddr), length(cLength), op(cOpType) {}
	~MemoryOpRequest() {}
	ReqOperation getOperation() const { return op; }
	bool isRead() const { return op == READ; }
	bool isWrite() const { return op == WRITE; }
	uint64_t getAddress() const { return addr; }
	uint64_t getLength() const { return length; }

protected:
	uint64_t addr;
	uint64_t length;
	ReqOperation op;
};

class FenceOpRequest : public GeneratorRequest {
public:
	FenceOpRequest() : GeneratorRequest() {}
	~FenceOpRequest() {}
	ReqOperation getOperation() const { return REQ_FENCE; }
};

class RequestGenerator : public Module {

public:
	RequestGenerator( Component* owner, Params& params) {}
	~RequestGenerator() {}
	virtual void generate(MirandaRequestQueue* q) { }
	virtual bool isFinished() { return true; }
	virtual void completed() { }

};

}
}

#endif
