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


#ifndef _H_SST_CASSINI_DMA_STATE
#define _H_SST_CASSINI_DMA_STATE

#include <dmacmd.h>

namespace SST {
namespace Cassini {

class DMAEngineState {
public:
        DMAEngineState(DMACommand* cmd) :
                origCmd(cmd) {

                issuedBytes = 0;
                completedIssue = false;
        }

	~DMAEngineState() {

	}

        bool issueCompleted() const {
                return completedIssue;
        }

	bool allReadsIssued() const {
		return (issuedBytes == getCommandLength());
	}

        uint64_t getIssuedBytes() const {
                return issuedBytes;
        }

        void setIssuedBytes(const uint64_t isBytes) {
                issuedBytes = isBytes;
        }

        void addIssuedBytes(const uint64_t addTo) {
                issuedBytes += addTo;
        }

        uint64_t getCommandLength() const {
                return cmd->getLength();
        }

        uint64_t getSrcAddr() const {
                return cmd->getSrcAddr();
        }

        uint64_t getDestAddr() const {
                return cmd->getDestAddr();
        }

        const DMACommand* getDMACommand() {
                return origCmd;
        }

private:
        const DMACommand* origCmd;
        uint64_t issuedBytes;
        bool completedIssue;
};

}
}

#endif
