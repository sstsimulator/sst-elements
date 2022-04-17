// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OS_WRITE_STATE
#define _H_VANADIS_OS_WRITE_STATE

#include <cstdio>
#include <functional>

#include "os/node/vnodeoshstate.h"
#include <sst/core/interfaces/stdMem.h>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisWriteHandlerState : public VanadisHandlerState {
public:
    VanadisWriteHandlerState(uint32_t verbosity, int file_descriptor, uint64_t buffer_addr, uint64_t buffer_count,
                             std::function<void(StandardMem::Request*)> send_r)
        : VanadisHandlerState(verbosity), file_descriptor(file_descriptor), write_buff(buffer_addr),
          write_count(buffer_count) {

        send_mem_req = send_r;
        total_written = 0;

        std_mem_handlers = new StandardMemHandlers(this, output);
    }

    ~VanadisWriteHandlerState() { delete std_mem_handlers; }

    virtual void handleIncomingRequest(StandardMem::Request* req) {
        req->handle(std_mem_handlers);
        
        if (total_written < write_count) {
            const uint64_t read_len = ((write_count - total_written) < 64) ? write_count - total_written : 64;

            send_mem_req(new StandardMem::Read(write_buff + total_written, read_len));
        } else {
            markComplete();
        }
    }

    class StandardMemHandlers : public StandardMem::RequestHandler {
    public:
        friend class VanadisWriteHandlerState;

        StandardMemHandlers(VanadisWriteHandlerState* state, SST::Output* out) :
            StandardMem::RequestHandler(out), state_handler(state) {}

        virtual ~StandardMemHandlers() {}
        
        virtual void handle(StandardMem::ReadResp* req) override {
            out->verbose(CALL_INFO, 16, 0,
                            "[syscall-write] processing incoming request (addr: "
                            "0x%llx, size: %" PRIu64 ")\n",
                            req->pAddr, (uint64_t)req->size);

            if (req->size > 0) {
                write( state_handler->file_descriptor, &req->data[0], req->size);
                state_handler->total_written += req->size;
            }
        }

    protected:
        VanadisWriteHandlerState* state_handler;
    };

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(total_written); }

protected:
    const uint64_t write_buff;
    const uint64_t write_count;

    uint64_t total_written;

    int file_descriptor;
    std::function<void(StandardMem::Request*)> send_mem_req;

    StandardMemHandlers* std_mem_handlers;
};

} // namespace Vanadis
} // namespace SST

#endif
