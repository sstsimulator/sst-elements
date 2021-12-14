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

#ifndef _H_VANADIS_OS_FSTAT_HANDLER
#define _H_VANADIS_OS_FSTAT_HANDLER

#include "os/node/vnodeoshstate.h"
#include <cstdint>
#include <functional>

namespace SST {
namespace Vanadis {

class VanadisFstatHandlerState : public VanadisHandlerState {
public:
    VanadisFstatHandlerState(uint32_t verbosity, int fd, uint64_t stat_a)
        : VanadisHandlerState(verbosity), file_handle(fd), stat_addr(stat_a) {

        completed = true;
    }

    virtual void handleIncomingRequest(StandardMem::Request* req) {
        output->verbose(CALL_INFO, 16, 0,
                        "-> [syscall-fstat] handle incoming event for fstat(), %s\n",
                        req->getString().c_str());
    }

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(0); }

private:
    const int32_t file_handle;
    const uint64_t stat_addr;
};

} // namespace Vanadis
} // namespace SST

#endif
