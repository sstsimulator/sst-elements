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

#ifndef _H_VANADIS_OS_OPENAT_HANDLER
#define _H_VANADIS_OS_OPENAT_HANDLER

#include <cinttypes>
#include <cstdint>

#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisOpenAtHandlerState : public VanadisHandlerState {
public:
    VanadisOpenAtHandlerState(uint32_t verbosity, int64_t dirfd, uint64_t path_ptr, int64_t flags,
                              std::unordered_map<uint32_t, VanadisOSFileDescriptor*>* fds,
                              std::function<void(StandardMem::Request*)> send_m)
        : VanadisHandlerState(verbosity), openat_dirfd(dirfd), openat_path_ptr(path_ptr), openat_flags(flags) {

        send_mem_req = send_m;
        opened_fd_handle = 3;

        std_mem_handlers = new StandardMemHandlers(this, output);
    }

    ~VanadisOpenAtHandlerState() { delete std_mem_handlers; }

    virtual void handleIncomingRequest(StandardMem::Request* req) {
        output->verbose(CALL_INFO, 16, 0, "[syscall-openat] request processing...\n");

        req->handle(std_mem_handlers);

        if (found_null) {
            output->verbose(CALL_INFO, 16, 0, "[syscall-openat] path at: \"%s\"\n", (const char*)&openat_path[0]);

            while (file_descriptors->find(opened_fd_handle) != file_descriptors->end()) {
                opened_fd_handle++;
            }

            output->verbose(CALL_INFO, 16, 0, "[syscall-openat] new descriptor at %" PRIu32 "\n", opened_fd_handle);

            file_descriptors->insert(std::pair<uint32_t, VanadisOSFileDescriptor*>(
                opened_fd_handle, new VanadisOSFileDescriptor(opened_fd_handle, (const char*)&openat_path[0])));

            markComplete();
        } else {
            send_mem_req(new StandardMem::Read(openat_path_ptr + openat_path.size(), 64));
        }
    }

    class StandardMemHandlers : public StandardMem::RequestHandler {
    public:
        StandardMemHandlers(VanadisOpenAtHandlerState* state, SST::Output* out) :
                StandardMem::RequestHandler(out), state_handler(state) {}

        virtual void handle(StandardMem::ReadResp* req) override {
            state_handler->found_null = false;

            for (size_t i = 0; i < req->size; ++i) {
                state_handler->openat_path.push_back(req->data[i]);

                if (req->data[i] == '\0') {
                    state_handler->found_null = true;
                }
            }
        }
    protected:
        VanadisOpenAtHandlerState* state_handler;
    };

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(opened_fd_handle); }

protected:
    const int64_t openat_dirfd;
    const uint64_t openat_path_ptr;
    const int64_t openat_flags;
    std::vector<uint8_t> openat_path;
    int32_t opened_fd_handle;
    std::unordered_map<uint32_t, VanadisOSFileDescriptor*>* file_descriptors;
    std::function<void(StandardMem::Request*)> send_mem_req;
    StandardMemHandlers* std_mem_handlers;
    bool found_null;
};

} // namespace Vanadis
} // namespace SST

#endif
