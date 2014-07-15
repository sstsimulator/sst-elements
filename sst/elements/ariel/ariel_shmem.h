// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ARIEL_SHMEM_H
#define SST_ARIEL_SHMEM_H 1

#include <inttypes.h>

#include <sst/core/interprocess/ipctunnel.h>

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
    ARIEL_ISSUE_TLM_FREE = 100,
    ARIEL_SWITCH_POOL = 110,
    ARIEL_NOOP = 128,
};

struct ArielCommand {
    ArielShmemCmd_t command;
    union {
        struct {
            uint32_t size;
            uint64_t addr;
        } inst;
        struct {
            uint64_t vaddr;
            uint64_t alloc_len;
            uint32_t alloc_level;
        } mlm_map;
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
    };
};


struct ArielSharedData {
    size_t numCores;
    uint64_t simTime;
    double timeConversion;
    uint8_t __pad[ 256 - sizeof(size_t) - sizeof(uint64_t) - sizeof(double)];
};




class ArielTunnel : public SST::Core::Interprocess::IPCTunnel<ArielSharedData, ArielCommand>
{
public:
    /**
     * Create a new Ariel Tunnel
     */
    ArielTunnel(const std::string &region_name, size_t numCores,
            size_t bufferSize, double timeConversion) :
        SST::Core::Interprocess::IPCTunnel<ArielSharedData, ArielCommand>(region_name, numCores, bufferSize)
    {
        sharedData->numCores = numCores;
        sharedData->simTime = 0;
        sharedData->timeConversion = timeConversion;
    }


    /**
     * Attach to an existing Ariel Tunnel (Created in another process
     */
    ArielTunnel(const std::string &region_name) :
        SST::Core::Interprocess::IPCTunnel<ArielSharedData, ArielCommand>(region_name)
    { }

    /** Update the current simulation cycle count in the SharedData region */
    void updateTime(uint64_t newTime) {
        sharedData->simTime = newTime;
    }

    /** Return the current time (in seconds) of the simulation */
    double getTime(void) {
        return sharedData->simTime * sharedData->timeConversion;
    }

    /** Set time conversion */
    void setTimeConversion(double x) {
        sharedData->timeConversion = x;
    }
};


}
}

#endif
