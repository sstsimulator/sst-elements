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

#ifndef _H_VANADIS_OS_ACCESS_HANDLER
#define _H_VANADIS_OS_ACCESS_HANDLER

#include "os/node/vnodeoshstate.h"
#include <cstdint>
#include <functional>

namespace SST {
namespace Vanadis {

class VanadisAccessHandlerState : public VanadisHandlerState {
public:
    VanadisAccessHandlerState(uint32_t verbosity, uint64_t ptr, uint64_t mode,
                              std::function<void(SimpleMem::Request*)> send_m)
        : VanadisHandlerState(verbosity), access_path_ptr(ptr), access_mode(mode) {

        send_mem_req = send_m;
        returnCode = 0;
    }

    virtual void handleIncomingRequest(SimpleMem::Request* req) {
        output->verbose(CALL_INFO, 16, 0,
                        "-> [syscall-access] handle incoming event for access(), "
                        "req->size: %" PRIu32 "\n",
                        (uint32_t)req->size);

        bool found_null = false;

        for (size_t i = 0; i < req->size; ++i) {
            access_path.push_back(req->data[i]);

            if (req->data[i] == '\0') {
                found_null = true;
                break;
            }
        }

        output->verbose(CALL_INFO, 16, 0, "-> [syscall-access] found null? %s\n", found_null ? "yes" : "no");

        if (found_null) {
            handlePathDecision((const char*)&access_path[0]);
        } else {
            send_mem_req(new SimpleMem::Request(SimpleMem::Request::Read, access_path_ptr + access_path.size(), 64));
        }
    }

    void handlePathDecision(const char* path) {
        output->verbose(CALL_INFO, 16, 0, "-> [syscall-access] path is: \"%s\"\n", path);

        if (strcmp(path, "/etc/suid-debug") == 0) {
            // Return ENOENT (which is 2, but we make it negative)
            returnCode = -2;
            markComplete();
        } else {
            output->fatal(CALL_INFO, -1, "Not sure what to do with path: \"%s\"\n", path);
        }

        output->verbose(CALL_INFO, 16, 0, "=> [syscall-access] return code set to: %" PRId64 "\n", returnCode);
    }

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(returnCode); }

protected:
    std::function<void(SimpleMem::Request*)> send_mem_req;
    std::vector<uint8_t> access_path;
    const uint64_t access_path_ptr;
    const uint64_t access_mode;

    int64_t returnCode;
};

} // namespace Vanadis
} // namespace SST

#endif
