// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <vector>
#include <algorithm>
#include "dmaEngine.h"
#include "util.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace SST::BalarComponent;

DMAEngine::DMAEngine(ComponentId_t id, Params &params) : SST::Component(id) {
    // Output for warnings
    out.init("DMAEngine[@f:@l:@p] ", params.find<int>("verbose", 1), 0, Output::STDOUT);

    // Clock and clock handler config
    std::string clockfreq = params.find<std::string>("clock", "1GHz");
    UnitAlgebra clock_ua(clockfreq);
    if (!(clock_ua.hasUnits("Hz") || clock_ua.hasUnits("s")) || clock_ua.getRoundedValue() <= 0) {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: clock. Must have units of Hz or s and be > 0. "
                "(SI prefixes ok). You specified '%s'\n", getName().c_str(), clockfreq.c_str());
    }
    TimeConverter* tc = getTimeConverter(clockfreq);

    // Bind tick function
    registerClock(tc, new Clock::Handler<DMAEngine>(this, &DMAEngine::tick));

    // MMIO Memory address and size
    mmio_addr = params.find<uint64_t>("mmio_addr", 0);
    mmio_size = params.find<uint32_t>("mmio_size", 512);

    // Get interfaces and bind handlers
    mmio_iface = loadUserSubComponent<SST::Interfaces::StandardMem>("mmio_iface", ComponentInfo::SHARE_NONE, tc, 
            new StandardMem::Handler<DMAEngine>(this, &DMAEngine::handleEvent));
    mem_iface = loadUserSubComponent<SST::Interfaces::StandardMem>("mem_iface", ComponentInfo::SHARE_NONE, tc, 
            new StandardMem::Handler<DMAEngine>(this, &DMAEngine::handleEvent));

    if (!mmio_iface) {
        out.fatal(CALL_INFO, -1, "%s, Error: No interface found loaded into 'mmio_iface' subcomponent slot. Please check input file\n", getName().c_str());
    }
    if (!mem_iface) {
        out.fatal(CALL_INFO, -1, "%s, Error: No interface found loaded into 'mem_iface' subcomponent slot. Please check input file\n", getName().c_str());
    }

    // Set MMIO address for dma Engine
    mmio_iface->setMemoryMappedAddressRegion(mmio_addr, mmio_size);

    // Initialize handlers
    handlers = new DMAHandlers(this, &out);
}

void DMAEngine::init(unsigned int phase) {
    mmio_iface->init(phase);
    mem_iface->init(phase);
}

void DMAEngine::setup() {
    mmio_iface->setup();
    mem_iface->setup();
}

/**
 * @brief Process through the DMA request queue
 * 
 * @param x 
 * @return true 
 * @return false 
 */
bool DMAEngine::tick(SST::Cycle_t x) {
    // Iterate through the DMA request queue to handle each request
    out.verbose(_INFO_, "%s: DMA request queue size: %ld\n", this->getName().c_str(), dma_requests.size());
    for (auto& it: dma_requests) {
        DMAEngineRequest_t *dma_req = it.first;
        out.verbose(_INFO_, "%s: DMA %s request status: %d\n", this->getName().c_str(), dmaRegToString(dma_req).c_str(), dma_req->status);

        if (dma_req->status == DMA_BUSY) {
            // Perform DMA copy

            // Make a request
            // Handle unaligned case
            uint32_t actual_transfer_size = dma_req->transfer_size;
            if (dma_req->data_size < dma_req->transfer_size)
                actual_transfer_size = dma_req->data_size;

            if (dma_req->dir == SIM_TO_SST) {
                // Easy, from simulator memory to SST memory
                // First prepare the data transfer block
                std::vector<uint8_t> data(actual_transfer_size);
                for (uint32_t i = 0; i < actual_transfer_size; i++) {
                    data[i] = *(dma_req->simulator_mem_addr + dma_req->offset + i);
                }

                StandardMem::Write* req = new StandardMem::Write(
                    dma_req->sst_mem_addr + dma_req->offset, actual_transfer_size, 
                    data, false, 0, dma_req->sst_mem_addr + dma_req->offset);


                // Advance offset
                dma_req->offset += actual_transfer_size;

                // If we are done, change the status to WAITING_DONE
                if (dma_req->offset >= dma_req->data_size) {
                    dma_req->status = DMA_WAITING_DONE;
                }

                // Increment ongoing count
                dma_req->ongoing_count++;

                // Send the copy request to memory system
                out.verbose(_INFO_, "%s: copy from simulator to SST mem space, reqid (%lld) writing vaddr: %llx paddr: %llx size: %d!\n", 
                            this->getName().c_str(), req->getID(), req->vAddr, req->pAddr, req->size);
                // Add the request to cache request map
                memory_requests[req->getID()] = dma_req;

                mem_iface->send(req);
            } else if (dma_req->dir == SST_TO_SIM) {
                // Need to first read it and get copy done inside readresp handler
                StandardMem::Read* req = new StandardMem::Read(
                    dma_req->sst_mem_addr + dma_req->offset, actual_transfer_size, 0, dma_req->sst_mem_addr + dma_req->offset);

                // Advance offset
                dma_req->offset += actual_transfer_size;

                // If we are done, change the status to WAITING_DONE
                if (dma_req->offset >= dma_req->data_size) {
                    dma_req->status = DMA_WAITING_DONE;
                }

                // Increment ongoing count
                dma_req->ongoing_count++;

                // Send the read request
                out.verbose(_INFO_, "%s: copy from SST to simulator mem space, reqid (%lld) reading vaddr: %llx paddr: %llx size: %d!\n", 
                            this->getName().c_str(), req->getID(), req->vAddr, req->pAddr, req->size);
                memory_requests[req->getID()] = dma_req;

                mem_iface->send(req);
            } else {
                out.fatal(CALL_INFO, -1, "%s: invalid DMA copy direction!\n", this->getName().c_str());
            }
        } else if (dma_req->status == DMA_WAITING_DONE) {
            // This means we have issued all required memory requests
            // Just waiting for those DMA memory transfers to be done
            // The DONE flag will be marked in writeresp handler if the data size is 0
            // and the ongoing count is 0
        } else if (dma_req->status == DMA_DONE) {
            // Clean up request and send response back
            delete dma_req;

            StandardMem::Write* pending_transfer = it.second;

            if (!(pending_transfer->posted)) {
                out.verbose(_INFO_, "%s: DMA done!\n", this->getName().c_str());
                mmio_iface->send(pending_transfer->makeResponse());
                delete pending_transfer;
            }

            // Set null pointer to indicate this request is done
            it.first = nullptr;
            it.second = nullptr;
        }
    }

    // Clean up the DMA request queue if the request is done
    dma_requests.erase(std::remove_if(dma_requests.begin(), 
                                      dma_requests.end(), 
                                      [](std::pair<DMAEngineRequest_t *, StandardMem::Write*>& x) { return x.first == nullptr; }), 
                                      dma_requests.end());

    // Should always enable DMA
    return false;
}

void DMAEngine::handleEvent(StandardMem::Request* req) {
    req->handle(handlers);
}

/**
 * @brief Handle write to the DMA control and status registers
 * 
 * @param write 
 */
void DMAEngine::DMAHandlers::handle(StandardMem::Write* write) {
    // Extract the DMA request from data payload
    DMAEngineControlRegisters* reg_ptr = decode_balar_packet<DMAEngineControlRegisters>(&(write->data)); 
    if (reg_ptr->data_size % reg_ptr->transfer_size != 0) {
        out->verbose(_INFO_, "%s: unaligned DMA transfer config! Data size: %ld Transfer size: %u\n", dma->getName().c_str(), reg_ptr->data_size, reg_ptr->transfer_size);
    }

    // Check if this request is overlapping with existing DMA requests in range for both start and end
    auto isAddrRangeOverlap = [](uint64_t start1, uint64_t end1, uint64_t start2, uint64_t end2) {
        return (start1 <= end2 && end1 >= start2);
    };

    auto isDMAReqOverlap = [&](DMAEngineControlRegisters* incoming, DMAEngineControlRegisters* existing) {
        return isAddrRangeOverlap(incoming->sst_mem_addr, incoming->sst_mem_addr + incoming->data_size, 
                                  existing->sst_mem_addr, existing->sst_mem_addr + existing->data_size) ||
               isAddrRangeOverlap((uint64_t) incoming->simulator_mem_addr, (uint64_t) incoming->simulator_mem_addr + incoming->data_size,
                                  (uint64_t) existing->simulator_mem_addr, (uint64_t) existing->simulator_mem_addr + existing->data_size);
    };

    for (auto& it: dma->dma_requests) {
        DMAEngineControlRegisters* existing_req = it.first;

        if (isDMAReqOverlap(reg_ptr, existing_req)) {
            out->fatal(CALL_INFO, -1, "%s: DMA request is overlapping with existing DMA request! Incoming: %s, Existing: %s\n", 
                       dma->getName().c_str(), dma->dmaRegToString(reg_ptr).c_str(), dma->dmaRegToString(existing_req).c_str());
        }
    }

    // Mark this DMA request as busy and push back to queue with the pending write
    out->verbose(_INFO_, "%s: DMA request received! %s\n", dma->getName().c_str(), dma->dmaRegToString(reg_ptr).c_str());
    reg_ptr->status = DMA_BUSY;
    reg_ptr->ongoing_count = 0;
    reg_ptr->offset = 0;
    dma->dma_requests.push_back({reg_ptr, write});

    // We will send the response back once DMA transfer is done in `tick` function
}

/**
 * @brief Handle response from copying sst mem space data to simulator mem space
 * 
 * @param read 
 */
void DMAEngine::DMAHandlers::handle(StandardMem::ReadResp* resp) {
    dma->out.verbose(_INFO_, "%s: readresp from SST mem space, reading vaddr: %llx paddr: %llx size: %d!\n", dma->getName().c_str(), resp->vAddr, resp->pAddr, resp->size);

    // Find the DMA request corresponds to this read by map
    auto it = dma->memory_requests.find(resp->getID());
    if (it == dma->memory_requests.end()) {
        dma->out.fatal(CALL_INFO, -1, "%s: DMA request not found for read response! ID: %lld\n", dma->getName().c_str(), resp->getID());
    }
    DMAEngineRequest_t *dma_req = it->second;

    // Find the simulator buffer pointer value by offset
    // of the sst mem space addr
    size_t offset = resp->pAddr - dma_req->sst_mem_addr;
    uint8_t * offseted_ptr = dma_req->simulator_mem_addr + offset;

    // Perform copy from response
    for (uint32_t i = 0; i < resp->size; i++) {
        *offseted_ptr = resp->data[i];
        offseted_ptr++;
    }

    // Mark as done for 1 mem req
    dma_req->ongoing_count--;
    dma->out.verbose(_INFO_, "%s: DMA %s ongoing count: %ld\n", dma->getName().c_str(), dma->dmaRegToString(dma_req).c_str(), dma_req->ongoing_count);

    // Check if this is the last mem request
    // Assuming the requests sent to memory are completed in order
    if (dma_req->status == DMA_WAITING_DONE &&
        dma_req->ongoing_count == 0) {
        dma_req->status = DMA_DONE;
    }

    dma->memory_requests.erase(it);
    delete resp;
}

/**
 * @brief Handle response from copying simulator mem space data to sst mem space 
 * 
 * @param write 
 */
void DMAEngine::DMAHandlers::handle(StandardMem::WriteResp* resp) {
    dma->out.verbose(_INFO_, "%s: writeresp from SST mem space, reading vaddr: %llx paddr: %llx size: %d!\n", dma->getName().c_str(), resp->vAddr, resp->pAddr, resp->size);
    
    // Find the DMA request corresponds to this read by map
    auto it = dma->memory_requests.find(resp->getID());
    if (it == dma->memory_requests.end()) {
        dma->out.fatal(CALL_INFO, -1, "%s: DMA request not found for read response! ID: %lld\n", dma->getName().c_str(), resp->getID());
    }
    DMAEngineRequest_t *dma_req = it->second;
    
    // Mark as done for 1 mem req
    dma_req->ongoing_count--;
    dma->out.verbose(_INFO_, "%s: DMA %s ongoing count: %ld\n", dma->getName().c_str(), dma->dmaRegToString(dma_req).c_str(), dma_req->ongoing_count);

    // Check if this is the last copy
    // Assuming the requests sent to memory are completed in order
    if (dma_req->status == DMA_WAITING_DONE &&
        dma_req->ongoing_count == 0) {
        dma_req->status = DMA_DONE;
    }

    dma->memory_requests.erase(it);
    delete resp;
}

/**
 * @brief Convert DMA request into a string with hex address range
 * 
 * @param reg_ptr 
 * @return std::string 
 */
std::string DMAEngine::dmaRegToString(DMAEngineControlRegisters* reg_ptr) {
    std::stringstream ss;
    std::string dir = reg_ptr->dir == SIM_TO_SST ? "<<" : "<<";
    ss << "Request size: 0x" << std::hex << reg_ptr->data_size << " sim: [0x" << std::hex << (uint64_t) reg_ptr->simulator_mem_addr << ", 0x" << std::hex << (uint64_t) reg_ptr->simulator_mem_addr + reg_ptr->data_size << "] " << dir << " sst: [0x" << std::hex << reg_ptr->sst_mem_addr << ", 0x" << std::hex << reg_ptr->sst_mem_addr + reg_ptr->data_size << "]";
    return ss.str();
}
