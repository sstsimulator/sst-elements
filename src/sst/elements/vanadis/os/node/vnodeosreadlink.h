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

#ifndef _H_VANADIS_OS_READLINK_STATE
#define _H_VANADIS_OS_READLINK_STATE

#include "os/node/vnodeoshstate.h"
#include <cstdio>
#include <sst/core/interfaces/stdMem.h>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisReadLinkHandlerState : public VanadisHandlerState {
public:
    VanadisReadLinkHandlerState(uint32_t verbosity, uint64_t path_buff, uint64_t out_buff, int64_t buff_size,
                                std::function<void(StandardMem::Request*)> send_mem,
                                std::function<void(const uint64_t, std::vector<uint8_t>&)> send_block)
        : VanadisHandlerState(verbosity), readlink_path_buff(path_buff), readlink_out_buff(out_buff),
          readlink_buff_size(buff_size) {

        send_mem_req = send_mem;
        send_block_mem = send_block;
        readlink_buff_bytes_written = 0;
        std_mem_handlers = new StandardMemHandlers(this, output);
    }

    ~VanadisReadLinkHandlerState() { delete std_mem_handlers; }

    virtual void handleIncomingRequest(StandardMem::Request* req) {
        output->verbose(CALL_INFO, 16, 0, "-> handle incoming for readlink()\n");

        req->handle(std_mem_handlers);

        if (found_null) {
            output->verbose(CALL_INFO, 16, 0, "path: %s\n", (char*)(&path[0]));

            if (strcmp((char*)(&path[0]), "/proc/self/exe") == 0) {
                const char* path_self = "/tmp/self";
                std::vector<uint8_t> real_path;

                for (int i = 0; i < strlen(path_self); ++i) {
                    real_path.push_back(path_self[i]);
                }

                output->verbose(CALL_INFO, 16, 0, "output: %s len: %" PRIu32 "\n", path_self,
                                (uint32_t)real_path.size());

                send_block_mem(readlink_out_buff, real_path);
                    readlink_buff_bytes_written = strlen(path_self);
            } else {
                output->fatal(CALL_INFO, -1, "Haven't implemented this yet.\n");
            }

            markComplete();
        } else {
            // Read the next cache line
            send_mem_req(new StandardMem::Read(readlink_path_buff + path.size(), 64));
        }
    }

    class StandardMemHandlers : public StandardMem::RequestHandler {
    public:
        friend class VanadisReadLinkHandlerState;

        StandardMemHandlers(VanadisReadLinkHandlerState* state, SST::Output* out) :
                StandardMem::RequestHandler(out), state_handler(state) {}
        
        virtual void handle(StandardMem::ReadResp* req) override {
            state_handler->found_null = false;

            for (size_t i = 0; i < req->size; ++i) {
                state_handler->path.push_back(req->data[i]);

                if (req->data[i] == '\0') {
                    state_handler->found_null = true;
                    break;
                }
            }
        }
    protected:
        VanadisReadLinkHandlerState* state_handler;
    };

    virtual VanadisSyscallResponse* generateResponse() {
        return new VanadisSyscallResponse(readlink_buff_bytes_written);
    }

protected:
    uint64_t readlink_path_buff;
    uint64_t readlink_out_buff;
    int64_t readlink_buff_size;
    int64_t readlink_buff_bytes_written;
    std::function<void(StandardMem::Request*)> send_mem_req;
    std::function<void(const uint64_t, std::vector<uint8_t>&)> send_block_mem;
    std::vector<uint8_t> path;

    StandardMemHandlers* std_mem_handlers;
    bool found_null;
};

} // namespace Vanadis
} // namespace SST

#endif
