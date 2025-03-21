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

#ifndef BALAR_DMA_ENGINE_H
#define BALAR_DMA_ENGINE_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>

#include "sst/elements/memHierarchy/util.h"

using namespace std;
using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

namespace SST {
namespace BalarComponent {
/* 
 * DMA Engine used in balar component
 */

class DMAEngine : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(DMAEngine, "balar", "dmaEngine", SST_ELI_ELEMENT_VERSION(1,0,0),
    "DMA Engine used in balar", COMPONENT_CATEGORY_PROCESSOR)
    SST_ELI_DOCUMENT_PARAMS( 
        {"verbose",                 "(uint) Determine how verbose the output from the device is", "0"},
        {"clock",                   "(UnitAlgebra/string) Clock frequency", "1GHz"},
        {"mmio_addr",               "(uint) Starting addr mapped to the device", "0"},
        {"mmio_size",               "(uint) Size of the MMIO memory range (Bytes)", "512"},
    )
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( 
        {"mmio_iface", "Command packet MMIO interface", "SST::Interfaces::StandardMem"},
        {"mem_iface", "Memory data packet interface", "SST::Interfaces::StandardMem"},
    )

    DMAEngine(ComponentId_t id, Params &params);
    ~DMAEngine() {}

    virtual void init(unsigned int);
    virtual void setup();
    void finish() {};
    bool tick(SST::Cycle_t x);

    /* DMA Registers */
    enum DMA_DIR {
        SIM_TO_SST = 0,
        SST_TO_SIM,
    };

    enum DMA_Status {
        DMA_FREE = 0,
        DMA_BUSY,
        DMA_WAITING_DONE,   // Waiting for mem to return
        DMA_DONE,
    };

    enum DMA_ERR {
        DMA_OK = 0,
        DMA_ERR_BUSY,
        DMA_ERR_INVALID,
    };

    /**
     * @brief DMA control and status registers
     *        Host need to pass a struct like this in write request
     *        to initiate a DMA transfer
     *        
     *        Once the transfer is done, status field will be updated
     * 
     */
    struct DMAEngineControlRegisters {
        uint8_t * simulator_mem_addr;
        Addr sst_mem_addr;
        size_t data_size;
        // The bytes count to copy each time
        // Sample values are like 4B or 8B
        // We will change the data_size and address
        // each time we perform a copy in DMA
        // Once the data_size is zero, we are done
        uint32_t transfer_size;
        // Use to track how many DMA requests are still waiting for mem response
        size_t ongoing_count;
        // Offset for sending the request to memory
        size_t offset;
        enum DMA_DIR dir;

        // Rest are status regs
        enum DMA_Status status;
        enum DMA_ERR errflag;
    };

    typedef struct DMAEngineControlRegisters DMAEngineRequest_t;

protected:
    /* Handle DMA requests from Host and responses from mem system */
    void handleEvent(StandardMem::Request* req);

    class DMAHandlers : public StandardMem::RequestHandler {
    public:
        friend class DMAEngine;
        DMAHandlers(DMAEngine* dma, SST::Output* out) : StandardMem::RequestHandler(out), dma(dma) {}
        virtual ~DMAHandlers() {}

        /**
         * @brief Pass in the simulator buffer address and size
         *        Also the SST memspace address and size
         *        Also the copy direction
         * 
         * @param write 
         */
        virtual void handle(StandardMem::Write* write) override;
        
        // Handlers for mem responses
        virtual void handle(StandardMem::ReadResp* resp) override;
        virtual void handle(StandardMem::WriteResp* resp) override;

    private:
        DMAEngine* dma;
    };

    /* Received ongoing DMA request queue */
    std::vector<std::pair<DMAEngineRequest_t *, StandardMem::Write*>> dma_requests;

    /* Memory requests associating with an active DMA request */
    std::map<StandardMem::Request::id_t, DMAEngineRequest_t *> memory_requests;

private:
    /* Output */
    Output out;

    /* MMIO config */
    Addr mmio_addr;
    uint32_t mmio_size;

    /* Command MMIO interface */
    StandardMem* mmio_iface;
    /* Memory access interface */
    StandardMem* mem_iface;

    /* Handlers */
    DMAHandlers* handlers;

    /* Save pending transfer and notify host once finishes */
    StandardMem::Write* pending_transfer;

    /* Convert DMA reg to string */
    std::string dmaRegToString(DMAEngineControlRegisters* reg_ptr);

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void emergencyShutdown() {};

}; // end DMAEngine

}
}

#endif // !BALAR_DMA_ENGINE_H
