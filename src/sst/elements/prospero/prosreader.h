// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_PROSPERO_READER
#define _H_SST_PROSPERO_READER

#include <sst/core/elementinfo.h>
#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
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

	bool isRead() const { return op == READ;  }
	bool isWrite() const { return op == WRITE; }
	uint64_t getAddress() const { return address; }
	uint32_t getLength() const { return length; }
	uint64_t getIssueAtCycle() const { return cycles; }
	ProsperoTraceEntryOperation getOperationType() const { return op; }
private:
	const uint64_t cycles;
	const uint64_t address;
	const uint32_t length;
	const ProsperoTraceEntryOperation op;
};

class ProsperoTraceReader : public SubComponent {

public:
	ProsperoTraceReader( Component* owner, Params& params ) : SubComponent(owner) {};
	~ProsperoTraceReader() { };
	virtual ProsperoTraceEntry* readNextEntry() { return NULL; };
	void setOutput(Output* out) { output = out; }

protected:
	Output* output;

};

}
}

#endif
