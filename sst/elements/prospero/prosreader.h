
#ifndef _H_SST_PROSPERO_READER
#define _H_SST_PROSPERO_READER

#include <sst/core/output.h>
#include <sst/core/module.h>
#include <sst/core/params.h>

namespace SST {
namespace Prospero {

typedef enum {
	READ,
	WRITE
} ProsperoTraceEntryOperation;

class ProsperoTraceEntry {
public:
	ProsperoTraceEntry(
		const uint64_t eCyc,
		const uint64_t eAddr,
		const uint32_t eLen,
		const ProsperoTraceEntryOperation eOp) :
		cycles(eCyc), address(eAddr), length(eLen), op(eOp) {

		}

	bool isRead()  { return op == READ;  }
	bool isWrite() { return op == WRITE; }
	uint64_t getAddress() { return address; }
	uint32_t getLength() { return length; }
	uint64_t getIssueAtCycle() { return cycles; }
	ProsperoTraceEntryOperation getOperationType() { return op; }
private:
	const uint64_t cycles;
	const uint64_t address;
	const uint32_t length;
	const ProsperoTraceEntryOperation op;
};

class ProsperoTraceReader : public Module {

public:
	ProsperoTraceReader( Component* owner, Params& params ) { }
	~ProsperoTraceReader() { }
	virtual ProsperoTraceEntry* readNextEntry() { return NULL; }

};

}
}

#endif
