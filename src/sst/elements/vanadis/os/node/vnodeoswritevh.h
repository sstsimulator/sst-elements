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

#ifndef _H_VANADIS_OS_WRITEV_STATE
#define _H_VANADIS_OS_WRITEV_STATE

#include <cstdio>
#include <vector>

#include "os/node/vnodeoshstate.h"
#include "os/vosbittype.h"
#include <sst/core/interfaces/stdMem.h>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisWritevHandlerState : public VanadisHandlerState {
public:
    VanadisWritevHandlerState(uint32_t verbosity, VanadisOSBitType bit_type, uint64_t iovec_addr, int64_t iovec_count, int file_descriptor,
                              std::function<void(StandardMem::Request*)> send_r)
        : VanadisHandlerState(verbosity), writev_iovec_addr(iovec_addr), writev_iovec_count(iovec_count),
          file_descriptor(file_descriptor), send_mem_req(send_r), bittype(bit_type) {

        reset_iovec();
        current_offset = 0;
        current_iovec = 0;
        total_bytes_written = 0;

        state = 0;

        std_mem_handlers = new StandardMemHandlers(this, output);
    }

    ~VanadisWritevHandlerState() { delete std_mem_handlers; }

    void reset_iovec() {
        current_iovec_base_addr = UINT64_MAX;
        current_iovec_length = INT64_MAX;
    }

    virtual void handleIncomingRequest(StandardMem::Request* req) {
        req->handle(std_mem_handlers);
        output->verbose(CALL_INFO, 16, 0,
                        "[syscall-writev] processing incoming request (addr: "
                        "0x%llx, size: %" PRIu64 ")\n",
                        resp_addr, (uint64_t)resp_size);
		  output->verbose(CALL_INFO, 16, 0, 
								"[syscall-writev] is OS 64bit? %s\n",
								bittype == VanadisOSBitType::VANADIS_OS_64B ? "yes" : "no");

        switch (state) {
        case 0: {
				switch(bittype) {
				case VanadisOSBitType::VANADIS_OS_32B:
					assert(4 == resp_size);
	            current_iovec_base_addr = (uint64_t)(*((uint32_t*)(&resp_data[0])));
					break;
				case VanadisOSBitType::VANADIS_OS_64B:
					assert(8 == resp_size);
	            current_iovec_base_addr = (uint64_t)(*((uint64_t*)(&resp_data[0])));
					break;
				}

            output->verbose(CALL_INFO, 16, 0, "iovec-data-address: 0x%llx\n", current_iovec_base_addr);

				switch(bittype) {
            case VanadisOSBitType::VANADIS_OS_32B:
	            send_mem_req(
   	             new StandardMem::Read(writev_iovec_addr + (current_iovec * 8) + resp_size, 4));
					break;
            case VanadisOSBitType::VANADIS_OS_64B:
	            send_mem_req(
   	             new StandardMem::Read(writev_iovec_addr + (current_iovec * 16) + resp_size, 8));
					break;
				}
            state++;
        } break;
        case 1: {
				switch(bittype) {
				case VanadisOSBitType::VANADIS_OS_32B:
					assert(4 == resp_size);
            current_iovec_length = (int64_t)(*((int32_t*)(&resp_data[0])));
				break;
				case VanadisOSBitType::VANADIS_OS_64B:
					assert(8 == resp_size);
            current_iovec_length = (int64_t)(*((int64_t*)(&resp_data[0])));
				break;
				}

            uint64_t base_addr_offset = (current_iovec_base_addr % 64);

            output->verbose(CALL_INFO, 16, 0, "iovec-data-len: %" PRIu64 "\n", current_iovec_length);

            if (current_iovec_length > 0) {
                if ((base_addr_offset + current_iovec_length) <= 64) {
                    // we only need to do one read and we are done
                    send_mem_req(new StandardMem::Read(current_iovec_base_addr,
                                                        current_iovec_length));
                    state++;
                } else {
                    send_mem_req(new StandardMem::Read(current_iovec_base_addr,
                                                        64 - base_addr_offset));
                    state++;
                }
            } else {
                current_iovec++;

                if (current_iovec < writev_iovec_count) {
                    current_offset = 0;

                    // Launch the next iovec read
							switch(bittype) {
				            case VanadisOSBitType::VANADIS_OS_32B:
            		        send_mem_req(
                  	      new StandardMem::Read(writev_iovec_addr + (current_iovec * 8), 4));
								break;
				            case VanadisOSBitType::VANADIS_OS_64B:
            		        send_mem_req(
                  	      new StandardMem::Read(writev_iovec_addr + (current_iovec * 16), 8));
								break;
							}
                    state = 0;
                } else {
                    output->verbose(CALL_INFO, 16, 0, "iovec processing is completed.\n");
                    printStatus();

                    state = 3;
                    dump_buffer();
                    markComplete();
                }
            }
        } break;
        case 2: {
            // Write out the payload
            output->verbose(CALL_INFO, 16, 0,
                            "--> update buffer data-offset: %" PRIu64 " + payload: %" PRIu64
                            " (iovec-data-len: %" PRIu64 ")\n",
                            current_offset, (uint64_t)resp_size, current_iovec_length);

            merge_to_buffer(resp_data);
            current_offset += resp_size;

            if (current_offset < current_iovec_length) {
                if ((current_offset + 64) < current_iovec_length) {
                    send_mem_req(
                        new StandardMem::Read(current_iovec_base_addr + current_offset, 64));
                } else {
                    uint64_t remainder = current_iovec_length - current_offset;
                    send_mem_req(new StandardMem::Read(current_iovec_base_addr + current_offset, remainder));
                }
            } else {
                current_iovec++;

                if (current_iovec < writev_iovec_count) {
                    current_offset = 0;

                    // Launch the next iovec read
							switch(bittype) {
                        case VanadisOSBitType::VANADIS_OS_32B:
		                    send_mem_req(
      		                  new StandardMem::Read(writev_iovec_addr + (current_iovec * 8), 4));
								break;
                        case VanadisOSBitType::VANADIS_OS_64B:
		                    send_mem_req(
      		                  new StandardMem::Read(writev_iovec_addr + (current_iovec * 16), 8));
								break;
							}

                    state = 0;
                } else {
                    output->verbose(CALL_INFO, 16, 0, "iovec processing is completed.\n");
                    printStatus();

                    state = 3;
                    dump_buffer();
                    markComplete();
                }
            }
        } break;
        case 3: {
            dump_buffer();

            // We are done here, don't do anything.
            markComplete();
        } break;
        }
    }

    class StandardMemHandlers : public StandardMem::RequestHandler {
    public:
        StandardMemHandlers(VanadisWritevHandlerState* state, SST::Output* out) :
                StandardMem::RequestHandler(out), state_handler(state) {}

        virtual void handle(StandardMem::ReadResp* req) override {
            state_handler->resp_data = req->data;
            state_handler->resp_size = req->size;
            state_handler->resp_addr = req->pAddr;
        }
    protected:
        VanadisWritevHandlerState* state_handler;
    };

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(total_bytes_written); }

    void merge_to_buffer(std::vector<uint8_t>& payload) {
        for (size_t i = 0; i < payload.size(); ++i) {
            buffer.push_back(payload[i]);
        }

        // print_buffer();
    }

    void clear_buffer() { buffer.clear(); }

    void dump_buffer() {
        if (buffer.size() > 0) {
            write( file_descriptor, &buffer[0], buffer.size() ); 			

            total_bytes_written += buffer.size();
        }

        buffer.clear();
    }

    void print_buffer() {
        for (size_t i = 0; i < buffer.size(); ++i) {
            printf("%c", buffer[i]);
        }

        printf("\n");
    }

    void printStatus() {
        output->verbose(CALL_INFO, 16, 0, "writev Handler Status\n");
        output->verbose(CALL_INFO, 16, 0, "-> iovec_addr:        0x%llx\n", writev_iovec_addr);
        output->verbose(CALL_INFO, 16, 0, "-> iovec_count:       %" PRId64 "\n", writev_iovec_count);
        output->verbose(CALL_INFO, 16, 0, "-> current iovec state:\n");
        output->verbose(CALL_INFO, 16, 0, "---> current_base:    0x%llx\n", current_iovec_base_addr);
        output->verbose(CALL_INFO, 16, 0, "---> current_len:     %" PRId64 "\n", current_iovec_length);
        output->verbose(CALL_INFO, 16, 0, "---> current_iovec:   %" PRId64 "\n", current_iovec);
        output->verbose(CALL_INFO, 16, 0, "---> current_offset:  %" PRId64 "\n", current_offset);
        output->verbose(CALL_INFO, 16, 0, "-> total-bytes:       %" PRId64 "\n", total_bytes_written);
        output->verbose(CALL_INFO, 16, 0, "-> current state:     %" PRId32 "\n", state);
    }

protected:
    uint64_t writev_iovec_addr;
    int64_t writev_iovec_count;

    uint64_t current_iovec_base_addr;
    int64_t current_iovec_length;

    int64_t current_iovec;
    int64_t current_offset;

    int64_t total_bytes_written;
    int32_t state;

    int file_descriptor;
    std::function<void(StandardMem::Request*)> send_mem_req;
    std::vector<uint8_t> buffer;

    StandardMemHandlers* std_mem_handlers;
    std::vector<uint8_t> resp_data;
    size_t resp_size;
    uint64_t resp_addr;

	VanadisOSBitType bittype;
};

} // namespace Vanadis
} // namespace SST

#endif
