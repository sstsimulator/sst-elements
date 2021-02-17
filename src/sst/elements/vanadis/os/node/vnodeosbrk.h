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

#ifndef _H_VANADIS_OS_BRK_HANDLER
#define _H_VANADIS_OS_BRK_HANDLER

#include <cstdint>
#include <functional>
#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisBRKHandlerState : public VanadisHandlerState {
public:
        VanadisBRKHandlerState( uint32_t verbosity, uint64_t return_v ) :
		VanadisHandlerState( verbosity ), rc(return_v) {

		completed = true;
        }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "-> [syscall-brk] handle incoming event for brk(), req->size: %" PRIu32 "\n",
			(uint32_t) req->size);
        }

	virtual VanadisSyscallResponse* generateResponse() {
		return new VanadisSyscallResponse( rc );
	}

protected:
	uint64_t rc;

};

}
}

#endif
