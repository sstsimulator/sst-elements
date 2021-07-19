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


#ifndef _H_SST_CASSINI_DMA_COMMAND
#define _H_SST_CASSINI_DMA_COMMAND

#include <sst/core/event.h>
#include <sst/core/component.h>

namespace SST {
namespace Cassini {

typedef std::pair<uint64_t, uint64_t> DMACommandID;

class DMACommand : public Event {

public:
	DMACommand(const Component* reqComp,
		const uint64_t dst,
		const uint64_t src,
		const uint64_t size) :
		Event(),
		destAddr(dst), srcAddr(src), length(size) {

		cmdID = std::make_pair(nextCmdID++, reqComp->getId());
		returnLinkID = 0;
	}

	uint64_t getSrcAddr() const { return srcAddr; }
	uint64_t getDestAddr() const { return destAddr; }
	uint64_t getLength() const { return length; }
	DMACommandID getCommandID() const { return cmdID; }
	void setCommandID(DMACommandID newID) { cmdID = newID; }

protected:
	static uint64_t nextCmdID;
	DMACommandID cmdID;
	const uint64_t destAddr;
	const uint64_t srcAddr;
	const uint64_t length;

	DMACommand() {} // For serialization
};

}
}

#endif
