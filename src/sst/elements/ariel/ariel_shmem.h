// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ARIEL_SHMEM_H
#define SST_ARIEL_SHMEM_H 1

#include <inttypes.h>

#include <sst/core/interprocess/ipctunnel.h>
#include "ariel_inst_class.h"

#define ARIEL_MAX_PAYLOAD_SIZE 64

namespace SST {
namespace ArielComponent {

enum ArielShmemCmd_t {
    ARIEL_PERFORM_EXIT = 1,
    ARIEL_PERFORM_READ = 2,
    ARIEL_PERFORM_WRITE = 4,
    ARIEL_START_DMA = 8,
    ARIEL_WAIT_DMA = 16,
    ARIEL_START_INSTRUCTION = 32,
    ARIEL_END_INSTRUCTION = 64,
    ARIEL_ISSUE_TLM_MAP = 80,
    ARIEL_ISSUE_TLM_MMAP = 81,
    ARIEL_ISSUE_TLM_MUNMAP = 82,
    ARIEL_ISSUE_TLM_FENCE = 83,
    ARIEL_ISSUE_TLM_FREE = 100,
    ARIEL_SWITCH_POOL = 110,
    ARIEL_NOOP = 128,
    ARIEL_OUTPUT_STATS = 140,
    ARIEL_FLUSHLINE_INSTRUCTION = 154,
    ARIEL_FENCE_INSTRUCTION = 155,
};

struct ArielCommand {
    ArielShmemCmd_t command;
    uint64_t instPtr;
    union {
        struct {
            uint32_t size;
            uint64_t addr;
            uint32_t instClass;
            uint32_t simdElemCount;
	    	uint8_t  payload[ARIEL_MAX_PAYLOAD_SIZE];
        } inst;
        struct {
            uint64_t vaddr;
            uint64_t alloc_len;
            uint32_t alloc_level;
        } mlm_map;
            struct {
            uint64_t vaddr;
            uint64_t alloc_len;
            uint32_t alloc_level;
            uint32_t fileID;
        } mlm_mmap;
            struct {
            uint64_t vaddr;
            uint64_t alloc_len;
            uint32_t alloc_level;
            uint32_t fileID;
        } mlm_munmap;
        struct{
            uint64_t vaddr;
        } mlm_fence;
        struct {
            uint64_t vaddr;
        } mlm_free;
        struct {
            uint32_t pool;
        } switchPool;
        struct {
            uint64_t src;
            uint64_t dest;
            uint32_t len;
        } dma_start;
        struct {
            uint64_t vaddr;
        } flushline;
    };
};


struct ArielSharedData {
    size_t numCores;
    uint64_t simTime;
    uint64_t cycles;
    volatile uint32_t child_attached;
    uint8_t __pad[ 256 - sizeof(uint32_t) - sizeof(size_t) - sizeof(uint64_t) - sizeof(uint64_t)];
};




class ArielTunnel : public SST::Core::Interprocess::IPCTunnel<ArielSharedData, ArielCommand>
{
public:
    /**
     * Create a new Ariel Tunnel
     */
    ArielTunnel(uint32_t comp_id, size_t numCores, size_t bufferSize) :
        SST::Core::Interprocess::IPCTunnel<ArielSharedData, ArielCommand>(comp_id, numCores, bufferSize) {
        sharedData->numCores = numCores;
        sharedData->simTime = 0;
        sharedData->cycles = 0;
        sharedData->child_attached = 0;
    }


    /**
     * Attach to an existing Ariel Tunnel (Created in another process
     */
    ArielTunnel(const std::string &region_name) :
        SST::Core::Interprocess::IPCTunnel<ArielSharedData, ArielCommand>(region_name) {
        /* Ideally, this would be done atomically, but we'll only have 1 child */
        sharedData->child_attached++;
    }

    void waitForChild(void) {
        while ( sharedData->child_attached == 0 ) ;
    }

    /** Update the current simulation cycle count in the SharedData region */
    void updateTime(uint64_t newTime) {
        sharedData->simTime = newTime;
    }

    /** Increment current cycle count */
    void incrementCycles() {
        sharedData->cycles++;
    }

    uint64_t getCycles() const {
        return sharedData->cycles;
    }

    /** Return the current time (in seconds) of the simulation */
    void getTime(struct timeval *tp) {
        uint64_t cTime = sharedData->simTime;
        tp->tv_sec = cTime / 1e9;
        tp->tv_usec = (cTime - (tp->tv_sec * 1e9)) / 1e3;
    }

    /** Return the current time in nanoseconds of the simulation */
    void getTimeNs(struct timespec *tp) {
        uint64_t cTime = sharedData->simTime;
        tp->tv_sec = cTime / 1e9;
        tp->tv_nsec = cTime - (tp->tv_sec * 1e9);
    }

};


}
}

#endif
