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

#ifndef _H_VANADIS_OS_OPEN_HANDLER
#define _H_VANADIS_OS_OPEN_HANDLER

#include <cinttypes>
#include <cstdint>
#include <fcntl.h>

#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisOpenHandlerState : public VanadisHandlerState {
public:
    VanadisOpenHandlerState(uint32_t verbosity, uint64_t path_ptr, uint64_t flags, uint64_t mode,
                            std::unordered_map<uint32_t, VanadisOSFileDescriptor*>* fds,
                            std::function<void(StandardMem::Request*)> send_m)
        : VanadisHandlerState(verbosity), open_path_ptr(path_ptr), open_flags(flags), open_mode(mode),
          file_descriptors(fds) {

        send_mem_req = send_m;
        opened_fd_handle = 3;

        std_mem_handlers = new StandardMemHandlers(this, output);
    }

    ~VanadisOpenHandlerState() { delete std_mem_handlers; }

    virtual void handleIncomingRequest(StandardMem::Request* req) {
        output->verbose(CALL_INFO, 16, 0, "[syscall-open] request processing...\n");

        // This function sets 'found_null'
        req->handle(std_mem_handlers);

        if (found_null) {
            const char* open_path_cstr = (const char*)&open_path[0];

            output->verbose(CALL_INFO, 16, 0, "[syscall-open] path: \"%s\", open_flags: %#x, open_mode: %#x\n", open_path_cstr,open_flags,open_mode);

            while (file_descriptors->find(opened_fd_handle) != file_descriptors->end()) {
                opened_fd_handle++;
            }

            try {
                VanadisOSFileDescriptor* desp = new VanadisOSFileDescriptor(open_path_cstr, open_flags, open_mode);
                file_descriptors->insert(std::pair<uint32_t, VanadisOSFileDescriptor*>( opened_fd_handle, desp ));
                output->verbose(CALL_INFO, 16, 0, "[syscall-open] new Vanadis file descriptor %" PRIu32 ", SST file descriptor %d\n",
                        opened_fd_handle, desp->getFileDescriptor() );
            } catch ( int error ) {
                opened_fd_handle = -errno;
                char buf[100];
                output->verbose(CALL_INFO, 16, 0, "[syscall-open] open of %s failed, `%s`\n", open_path_cstr, strerror_r(errno,buf,100) );
           }

            markComplete();
        } else {
            send_mem_req(new StandardMem::Read(open_path_ptr + open_path.size(), 64));
        }
    }

    class StandardMemHandlers : public StandardMem::RequestHandler {
    public:
        StandardMemHandlers(VanadisOpenHandlerState* state, SST::Output* out) :
            StandardMem::RequestHandler(out), state_handler(state) {}
        
        virtual void handle(StandardMem::ReadResp* req) override {

            state_handler->found_null = false;

            for (size_t i = 0; i < req->size; ++i) {
                state_handler->open_path.push_back(req->data[i]);

                if (req->data[i] == '\0') {
                    state_handler->found_null = true;
                }
            }
        }
    protected:
        VanadisOpenHandlerState* state_handler;
    };

    virtual VanadisSyscallResponse* generateResponse() {
        VanadisSyscallResponse* resp = new VanadisSyscallResponse(opened_fd_handle);

        if (opened_fd_handle < 0) {
            resp->markFailed();
        }

        return resp;
    }

protected:
    const uint64_t open_path_ptr;
    const uint64_t open_flags;
    const uint64_t open_mode;

    std::vector<uint8_t> open_path;
    int32_t opened_fd_handle;
    std::unordered_map<uint32_t, VanadisOSFileDescriptor*>* file_descriptors;
    std::function<void(StandardMem::Request*)> send_mem_req;
    bool found_null;
    StandardMemHandlers* std_mem_handlers;
};

} // namespace Vanadis
} // namespace SST

#endif
