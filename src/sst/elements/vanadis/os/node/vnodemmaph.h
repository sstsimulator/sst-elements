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

#ifndef _H_VANADIS_OS_MMAP_STATE
#define _H_VANADIS_OS_MMAP_STATE

#include <cassert>
#include <cstdio>
#include <functional>

#include "os/node/vnodeoshstate.h"
#include <sst/core/interfaces/simpleMem.h>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisMemoryMapHandlerState : public VanadisHandlerState {
public:
    VanadisMemoryMapHandlerState(uint32_t verbosity, uint64_t address, uint64_t length, int64_t prot_flg,
                                 int64_t map_flg, uint64_t stack_p, uint64_t offset_size,
                                 std::function<void(SimpleMem::Request*)> send_mem_r,
                                 std::function<void(const uint64_t, std::vector<uint8_t>&)> send_block,
                                 VanadisMemoryManager* mem_mgr)
        : VanadisHandlerState(verbosity), map_address(address), map_length(length), map_protect(prot_flg),
          map_flags(map_flg), call_stack(stack_p), offset_units(offset_size), memory_mgr(mem_mgr) {

        send_block_mem = send_block;
        send_mem_req = send_mem_r;
        state = 0;
        return_value = 0;
    }

    virtual void handleIncomingRequest(SimpleMem::Request* req) {
        output->verbose(CALL_INFO, 16, 0,
                        "[syscall-mmap] processing incoming request (addr: 0x%llx, "
                        "size: %" PRIu64 ")\n",
                        req->addr, (uint64_t)req->size);

        switch (state) {
        case 0: {
            assert(req->size >= 4);
            file_descriptor = (int64_t)(*((int32_t*)&req->data[0]));

            output->verbose(CALL_INFO, 16, 0, "[syscall-mmap] -> set file descriptor to %" PRId64 "\n",
                            file_descriptor);

            send_mem_req(new SimpleMem::Request(SimpleMem::Request::Read, req->addr + req->size, 4));
            state++;
        } break;

        case 1: {
            assert(req->size >= 4);
            file_offset = (uint64_t)(*((int32_t*)&req->data[0]));

            output->verbose(CALL_INFO, 16, 0, "[syscall-mmap] -> set file offset to %" PRIu64 "\n", file_offset);
            output->verbose(CALL_INFO, 16, 0,
                            "[syscall-mmap] -> mmap( 0x%llx, %" PRIu64 ", %" PRId64 ", %" PRId64 ", %" PRId64
                            ", %" PRIu64 " ) file-offset-units: %" PRIu64 "\n",
                            map_address, map_length, map_protect, map_flags, file_descriptor, file_offset,
                            offset_units);

            uint64_t allocation_start = 0;

            // Check if the allocation request is MAP_ANONYMOUS in which case we
            // ignore the file descriptor and offsets
            if ((map_flags & 0x800) != 0x0) {
                output->verbose(CALL_INFO, 16, 0, "[syscall-mmap] --> calling memory manager to allocate pages...\n");

                if (0 == map_address) {
                    int status = memory_mgr->allocateRange(map_length, &allocation_start);

                    output->verbose(CALL_INFO, 16, 0,
                                    "[syscall-mmap] ---> memory manager status: %d / "
                                    "allocation_start: 0x%llx\n",
                                    status, allocation_start);

                    if (0 == status) {
                        std::vector<uint8_t> payload;
                        payload.resize(map_length, 0);
                        send_block_mem(allocation_start, payload);

                        return_value = (int64_t)allocation_start;
                    } else {
                        return_value = -22;
                    }
                } else {
                    output->verbose(CALL_INFO, 16, 0,
                                    "[syscall-mmap] ---> requested address 0x%llx "
                                    "automatically returned.\n",
                                    map_address);

                    std::vector<uint8_t> payload;
                    payload.resize(map_length, 0);
                    send_block_mem(map_address, payload);

                    return_value = (int64_t)map_address;
                }
            } else {
                return_value = -22;
            }

            markComplete();
            state++;
        } break;
        default: {
            // Nothing to do here?
        } break;
        }
    }

    virtual VanadisSyscallResponse* generateResponse() {
        VanadisSyscallResponse* resp = new VanadisSyscallResponse(return_value);

        if (return_value < 0) {
            resp->markFailed();
        }

        return resp;
    }

protected:
    const uint64_t map_address;
    const uint64_t map_length;
    const int64_t map_protect;
    const int64_t map_flags;
    int64_t file_descriptor;
    uint64_t file_offset;
    const uint64_t call_stack;
    const uint64_t offset_units;
    std::function<void(SimpleMem::Request*)> send_mem_req;
    std::function<void(const uint64_t, std::vector<uint8_t>&)> send_block_mem;

    int64_t return_value;
    int state;

    VanadisMemoryManager* memory_mgr;
};

} // namespace Vanadis
} // namespace SST

#endif
