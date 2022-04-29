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

#ifndef _H_VANADIS_OS_READV_STATE
#define _H_VANADIS_OS_READV_STATE

#include <cstdio>
#include <vector>

#include "os/node/vnodeoshstate.h"
#include "os/vosbittype.h"
#include <sst/core/interfaces/stdMem.h>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisReadvHandlerState : public VanadisHandlerState {

    struct osIovec {
       uint64_t iov_base;
       uint64_t iov_len;
    };

public:
    VanadisReadvHandlerState(uint32_t verbosity, VanadisOSBitType bit_type, uint64_t iovec_addr, int64_t iovec_count, int file_descriptor,
                              std::function<void(StandardMem::Request*)> send_r)

        : VanadisHandlerState(verbosity), iovec_addr(iovec_addr), iovec_count(iovec_count),
          file_descriptor(file_descriptor), send_mem_req(send_r), bittype(bit_type), current_iovec(0), total_bytes_read(0), iovBufferOffset(0) {

        size_t len = getIovecStructLen(bittype) * iovec_count;
        
        output->verbose(CALL_INFO, 16, 0, "[syscall-readv] address of iovec %#" PRIx64 " totalBytes=%zu\n", iovec_addr, len );

        // read in the all iovec's
        // align first read on cache line
        uint64_t memReadLen = (iovec_addr % 64) == 0 ? 64 : 64 - ( iovec_addr & (64-1) );
        memReadLen = memReadLen > len ? len : memReadLen;

        iovBuffer.resize( len );

        // read first chunk of iovec (could be all if fits in cache line)
        output->verbose(CALL_INFO, 16, 0, "[syscall-readv]  read %" PRIu64 " bytes of iovec addr=%#" PRIx64 "\n", memReadLen, iovec_addr );
        send_mem_req(new StandardMem::Read(iovec_addr, memReadLen ) );

        // set up the handler for this read
        handler = std::bind( &VanadisReadvHandlerState::handleIovecReadMemResp, this );

        std_mem_handlers = new StandardMemHandlers(this, output);
    }

    ~VanadisReadvHandlerState() { delete std_mem_handlers; }

    virtual void handleIncomingRequest(StandardMem::Request* req) 
    {
        req->handle(std_mem_handlers);
        
        output->verbose(CALL_INFO, 16, 0,
                        "[syscall-readv] processing incoming request (addr: "
                        "0x%llx, size: %" PRIu64 ")\n",
                        resp_addr, (uint64_t)resp_size);
        output->verbose(CALL_INFO, 16, 0, 
                        "[syscall-readv] is OS 64bit? %s\n",
                        bittype == VanadisOSBitType::VANADIS_OS_64B ? "yes" : "no");
        handler();
    }

    void handleIovecReadMemResp() 
    {
        memcpy( iovBuffer.data() + iovBufferOffset, resp_data.data(), resp_size );
        iovBufferOffset += resp_size;

        if ( iovBufferOffset < iovBuffer.size() ) {
            size_t memReadLen = iovBuffer.size() - iovBufferOffset > 64 ? 64 : iovBuffer.size() - iovBufferOffset; 

            output->verbose(CALL_INFO, 16, 0, "[syscall-readv]  read %zu bytes of iovec addr=%#" PRIx64 "\n", memReadLen, iovec_addr + iovBufferOffset );
            send_mem_req(new StandardMem::Read(iovec_addr + iovBufferOffset, memReadLen ) );
        } else {
            output->verbose(CALL_INFO, 16, 0, "[syscall-readv] have read all of the iovec from memory\n");
            for ( int i =0; i < iovec_count; i++ ) {
                output->verbose(CALL_INFO, 16, 0, "[syscall-readv] iovec[%d] addr=%#" PRIx64 " len=%zu\n", 
                    i, getIovecBase( iovBuffer, i, bittype ), getIovecLen( iovBuffer, i, bittype ) );
            }
            firstWriteOfBuffer();
        }
    } 
    void firstWriteOfBuffer() {

        while ( 0 == getIovecLen( iovBuffer, current_iovec, bittype ) && current_iovec  < iovec_count ) {
            output->verbose(CALL_INFO, 16, 0, "[syscall-readv] iovec[%" PRId64 "] addr=%#" PRIx64 " has len 0\n",
                    current_iovec, getIovecBase( iovBuffer, current_iovec, bittype ) );
            ++current_iovec;
        }

        currentIovecStruct.iov_base = getIovecBase( iovBuffer, current_iovec, bittype ); 
        currentIovecStruct.iov_len = getIovecLen( iovBuffer, current_iovec, bittype ); 

        output->verbose(CALL_INFO, 16, 0, "[syscall-readv] iovec[%" PRId64 "] addr=%#" PRIx64 " len=%zu\n", 
                    current_iovec, currentIovecStruct.iov_base, currentIovecStruct.iov_len );

        bufferOffset = 0;
        buffer.resize( currentIovecStruct.iov_len );

        // read of the file for this buffer
        numRead = read( file_descriptor, buffer.data(), currentIovecStruct.iov_len );   
        assert( numRead >= 0 );

        buffer.resize(numRead);

        output->verbose(CALL_INFO, 16, 0, "[syscall-readv] numRead=%zu\n", numRead );

        uint64_t memWriteLen = (currentIovecStruct.iov_base % 64) == 0 ? 64 : 64 - ( currentIovecStruct.iov_base & (64-1) );
        memWriteLen = memWriteLen > currentIovecStruct.iov_len ? currentIovecStruct.iov_len : memWriteLen;

        // write first chunk of the buffer (could be all if fits in cache line)
        output->verbose(CALL_INFO, 16, 0, "[syscall-readv] write %" PRIu64 " bytes to buffer addr=%#" PRIx64 "\n", memWriteLen, currentIovecStruct.iov_base );

        std::vector<uint8_t> payload = { buffer.begin(), buffer.begin() + memWriteLen };
        send_mem_req(new StandardMem::Write( currentIovecStruct.iov_base, memWriteLen, payload ) );

        handler = std::bind( &VanadisReadvHandlerState::handleBufferWriteMemResp, this );
    }

    void handleBufferWriteMemResp() {
        bufferOffset += resp_size;

        output->verbose(CALL_INFO, 16, 0, "[syscall-readv] resp_size=%zu\n", resp_size );

        total_bytes_read += resp_size;

        if ( bufferOffset < buffer.size() ) {
            size_t memWriteLen = buffer.size() - bufferOffset > 64 ? 64 : buffer.size() - bufferOffset; 
            output->verbose(CALL_INFO, 16, 0, "[syscall-readv] write %zu bytes to buffer addr=%#" PRIx64 "\n", memWriteLen, currentIovecStruct.iov_base + bufferOffset);
            std::vector<uint8_t> payload = { buffer.begin() + bufferOffset, buffer.begin() + bufferOffset + memWriteLen };
            send_mem_req(new StandardMem::Write( currentIovecStruct.iov_base + bufferOffset, memWriteLen, payload ) );
        } else {
            output->verbose(CALL_INFO, 16, 0, "[syscall-readv] buffer %" PRId64 " is done\n", current_iovec );
            ++current_iovec;
            if ( current_iovec  < iovec_count ) {
                firstWriteOfBuffer();     
            } else {
                output->verbose(CALL_INFO, 16, 0, "[syscall-readv] is done total_bytes_read %" PRId64 "\n", total_bytes_read );
                markComplete();
            }
        }
    }
        
    class StandardMemHandlers : public StandardMem::RequestHandler {
    public:
        StandardMemHandlers(VanadisReadvHandlerState* state, SST::Output* out) :
                StandardMem::RequestHandler(out), state_handler(state) {}

        virtual void handle(StandardMem::ReadResp* req) override {
            state_handler->resp_data = req->data;
            state_handler->resp_size = req->size;
            state_handler->resp_addr = req->pAddr;
        }
        virtual void handle(StandardMem::WriteResp* req) override {
            state_handler->resp_size = req->size;
            state_handler->resp_addr = req->pAddr;
        }
    protected:
        VanadisReadvHandlerState* state_handler;
    };

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(total_bytes_read); }

    uint64_t getIovecBase( std::vector<uint8_t>& data, int pos, VanadisOSBitType bit_type ) {
        switch(bit_type) {
          case VanadisOSBitType::VANADIS_OS_32B:
            return (uint64_t)(*((uint32_t*)(&data[pos*8])));

          case VanadisOSBitType::VANADIS_OS_64B:
            return (uint64_t)(*((uint32_t*)(&data[pos*16])));
          default:
            output->fatal(CALL_INFO, -1, "OS event is neither 32b or 64b, unsupported OS bit type.\n");
        }
        return 0; // Eliminate compile warning
    }

    size_t getIovecLen( std::vector<uint8_t>& data, int pos, VanadisOSBitType bit_type ) {
        switch(bit_type) {
          case VanadisOSBitType::VANADIS_OS_32B:
            return (uint64_t)(*((uint32_t*)(&data[pos*8+4])));

          case VanadisOSBitType::VANADIS_OS_64B:
            return (uint64_t)(*((uint32_t*)(&data[pos*16+8])));
          default:
            output->fatal(CALL_INFO, -1, "OS event is neither 32b or 64b, unsupported OS bit type.\n");
        }
        return 0; // Eliminate compile warning
    }


    int getIovecStructLen( VanadisOSBitType bit_type ) {
        switch(bit_type) {
          case VanadisOSBitType::VANADIS_OS_32B:
            return 4*2;
          case VanadisOSBitType::VANADIS_OS_64B:
            return 8*2;
          default:
            output->fatal(CALL_INFO, -1, "OS event is neither 32b or 64b, unsupported OS bit type.\n");
        }
        return 0; // Eliminate compile warning
    }

protected:
    std::function<void(void)> handler;

    struct osIovec currentIovecStruct;

    uint64_t    iovec_addr;
    int64_t     iovec_count;

    int64_t current_iovec;

    int64_t total_bytes_read;

    ssize_t numRead;
    int file_descriptor;
    std::function<void(StandardMem::Request*)> send_mem_req;

    size_t iovBufferOffset;
    std::vector<uint8_t> iovBuffer;

    std::vector<uint8_t> buffer;
    size_t bufferOffset;

    StandardMemHandlers* std_mem_handlers;
    std::vector<uint8_t> resp_data;
    size_t resp_size;
    uint64_t resp_addr;

	VanadisOSBitType bittype;
};

} // namespace Vanadis
} // namespace SST

#endif
