// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_CASSINI_DMA_MEMORY_OPERATION
#define _H_SST_CASSINI_DMA_MEMORY_OPERATION

namespace SST {
namespace Cassini {

class DMAMemoryOperation {

public:
	DMAMemoryOperation(const uint64_t byteOffset,
		const uint64_t len,
		const bool isReadOperation) :
		offset(byteOffset), length(len), isRead(isReadOperation) {}
	DMAMemoryOperation(const uint64_t byteOffset,
		const uint64_t len) :
		offset(byteOffset), length(len), isRead(true) {}

	~DMAMemoryOperation() {}
	uint64_t getByteOffset() const { return offset; }
	uint64_t getLength() const { return length; }
	bool isRead() const { return isRead; }
	bool isWrite() const { return isWrite; }

private:
	const uint64_t offset;
	const uint64_t length;
	const bool isRead;

};

}
}

#endif
