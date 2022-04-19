// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OS_UNLINKAT_HANDLER
#define _H_VANADIS_OS_UNLINKAT_HANDLER

#include <cinttypes>
#include <cstdint>
#include <fcntl.h>

#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisUnlinkatHandlerState : public VanadisHandlerState {
public:
    VanadisUnlinkatHandlerState(uint32_t verbosity, uint64_t fd, uint64_t path_ptr, uint64_t flags, std::function<void(StandardMem::Request*)> send_m)
        : VanadisHandlerState(verbosity), dirFd(fd), path_ptr(path_ptr), flags(flags), retval(0) {

        send_mem_req = send_m;

        std_mem_handlers = new StandardMemHandlers(this, output);
    }

    ~VanadisUnlinkatHandlerState() { delete std_mem_handlers; }

    virtual void handleIncomingRequest(StandardMem::Request* req) {
        output->verbose(CALL_INFO, 16, 0, "[syscall-unlinkat] request processing...\n");

        // This function sets 'found_null'
        req->handle(std_mem_handlers);

        if (found_null) {
            const char* path_cstr = (const char*)&path[0];

            output->verbose(CALL_INFO, 16, 0, "[syscall-unlinkat] path: \"%s\"\n", path_cstr);

printf("%d %s %x\n",dirFd,path_cstr,flags);
            if ( unlinkat( dirFd, path_cstr,flags ) ) {
                retval = -errno;
                char buf[100];
                strerror_r(errno,buf,100);
printf("%s\n",strerror_r(errno,buf,100));
                output->verbose(CALL_INFO, 16, 0, "[syscall-unlinkat] unlink of %s failed, `%s`\n", path_cstr, buf );
            }
            markComplete();
        } else {
            send_mem_req(new StandardMem::Read(path_ptr + path.size(), 64));
        }
    }

    class StandardMemHandlers : public StandardMem::RequestHandler {
    public:
        StandardMemHandlers(VanadisUnlinkatHandlerState* state, SST::Output* out) :
            StandardMem::RequestHandler(out), state_handler(state) {}
        
        virtual void handle(StandardMem::ReadResp* req) override {

            state_handler->found_null = false;

            for (size_t i = 0; i < req->size; ++i) {
                state_handler->path.push_back(req->data[i]);

                if (req->data[i] == '\0') {
                    state_handler->found_null = true;
                }
            }
        }
    protected:
        VanadisUnlinkatHandlerState* state_handler;
    };

    virtual VanadisSyscallResponse* generateResponse() {

        VanadisSyscallResponse* resp = new VanadisSyscallResponse( retval );

        if ( retval )  {
            resp->markFailed();
        }

        return resp;
    }

protected:
    const uint64_t dirFd;
    const uint64_t path_ptr;
    const uint64_t flags;

    std::vector<uint8_t> path;
    std::function<void(StandardMem::Request*)> send_mem_req;
    bool found_null;
    StandardMemHandlers* std_mem_handlers;
    int retval;
};

} // namespace Vanadis
} // namespace SST

#endif
