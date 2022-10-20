/**
 * @file dmaEngine.cc
 * @author Weili An (an107@purdue.edu)
 * @brief A DMA engine component to copy data from SST memspace to
 *        simulator memspace and vice versa
 * @version 0.1
 * @date 2022-10-18
 * 
 * 
 */

#include <sst_config.h>
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
    iface = loadUserSubComponent<SST::Interfaces::StandardMem>("iface", ComponentInfo::SHARE_NONE, tc, 
            new StandardMem::Handler<DMAEngine>(this, &DMAEngine::handleEvent));

    if (!iface) {
        out.fatal(CALL_INFO, -1, "%s, Error: No interface found loaded into 'iface' subcomponent slot. Please check input file\n", getName().c_str());
    }

    // Set MMIO address for dma Engine
    iface->setMemoryMappedAddressRegion(mmio_addr, mmio_size);

    // Initialize handlers
    handlers = new DMAHandlers(this, &out);
    
    // Initialize control&status reg
    dma_ctrl_regs.status = DMA_FREE;
    dma_ctrl_regs.errflag = DMA_OK;
}

void DMAEngine::init(unsigned int phase) {
    iface->init(phase);
}

void DMAEngine::setup() {
    iface->setup();
}

/**
 * @brief Advance DMA Transfer according to the passed in
 *        control regs
 * 
 * @param x 
 * @return true 
 * @return false 
 */
bool DMAEngine::tick(SST::Cycle_t x) {
    if (dma_ctrl_regs.status == DMA_BUSY) {
        // Perform DMA copy

        // Make a request
        if (dma_ctrl_regs.dir == SIM_TO_SST) {
            // Easy, from simulator memory to SST memory
            // First prepare the data transfer block
            std::vector<uint8_t> data(dma_ctrl_regs.transfer_size);
            for (uint32_t i = 0; i < dma_ctrl_regs.transfer_size; i++) {
                data[i] = *(dma_ctrl_regs.simulator_mem_addr);
                dma_ctrl_regs.simulator_mem_addr++;
            }

            StandardMem::Write* req = new StandardMem::Write(
                dma_ctrl_regs.sst_mem_addr, dma_ctrl_regs.transfer_size, 
                data, false);

            // Increase the SST mem space address for next copy
            dma_ctrl_regs.sst_mem_addr += dma_ctrl_regs.transfer_size;

            // Reduce the total data_size for record
            dma_ctrl_regs.data_size -= dma_ctrl_regs.transfer_size;

            // If we are done, change the status to WAITING_DONE
            if (dma_ctrl_regs.data_size == 0) {
                dma_ctrl_regs.status = DMA_WAITING_DONE;
            }

            // Send the copy request
            iface->send(req);
        } else if (dma_ctrl_regs.dir == SST_TO_SIM) {
            // Need to first read it and get copy done inside readresp handler
            StandardMem::Read* req = new StandardMem::Read(
                dma_ctrl_regs.sst_mem_addr, dma_ctrl_regs.transfer_size);

            // Increase the SST mem space address and simulator mem space addr
            // for next copy
            // Also decrease the data size
            dma_ctrl_regs.sst_mem_addr += dma_ctrl_regs.transfer_size;
            dma_ctrl_regs.simulator_mem_addr += dma_ctrl_regs.transfer_size;
            dma_ctrl_regs.data_size -= dma_ctrl_regs.transfer_size;

            // If we are done, change the status to WAITING_DONE
            if (dma_ctrl_regs.data_size == 0) {
                dma_ctrl_regs.status = DMA_WAITING_DONE;
            }

            // Send the copy request
            iface->send(req);
        } else {
            out.fatal(CALL_INFO, -1, "%s: invalid DMA copy direction!\n", this->getName().c_str());
        }

        return false;
    } else if (dma_ctrl_regs.status == DMA_WAITING_DONE) {
        // Just waiting for the DMA memory transfer to be done
        // The DONE flag will be marked in writeresp handler if the data size is 0
        return false;
    } else if (dma_ctrl_regs.status == DMA_DONE) {
        // Reset error flag and send back response
        dma_ctrl_regs.status = DMA_FREE;
        dma_ctrl_regs.errflag = DMA_OK;

        if (!(pending_transfer->posted)) {
            out.verbose(_INFO_, "%s: DMA done!\n", this->getName().c_str());
            iface->send(pending_transfer->makeResponse());
            delete pending_transfer;
        }

        return false;
    } else if (dma_ctrl_regs.status == DMA_FREE) {
        // Do nothing and wait since we dont have any task to do
        return false;
    }
}

void DMAEngine::handleEvent(StandardMem::Request* req) {
    req->handle(handlers);
}

/**
 * @brief Handle read to the DMA control&status registers
 * 
 * @param read 
 */
void DMAEngine::DMAHandlers::handle(StandardMem::Read* read) {
    StandardMem::ReadResp* resp = static_cast<StandardMem::ReadResp*> (read->makeResponse());
    
    std::vector<uint8_t> *payload = encode_balar_packet<DMAEngineControlRegisters>(&(dma->dma_ctrl_regs));
    payload->resize(read->size, 0);
    resp->data = *payload;

    dma->iface->send(resp);
    delete payload;
}

/**
 * @brief Handle write to the DMA control and status registers
 * 
 * @param write 
 */
void DMAEngine::DMAHandlers::handle(StandardMem::Write* write) {
    if (dma->dma_ctrl_regs.status != DMA_FREE) {
        // DMA busy, need to set err flag
        dma->dma_ctrl_regs.errflag = DMA_ERR_BUSY;
        out->verbose(_INFO_, "%s: issued request while DMA busy!\n", dma->getName().c_str());

        if (!(write->posted)) {
            dma->iface->send(write->makeResponse());
            delete write;
        }
    } else {
        // Save pending transfer
        dma->pending_transfer = write;

        // Write the control status register
        DMAEngineControlRegisters* reg_ptr = decode_balar_packet<DMAEngineControlRegisters>(&(write->data)); 
        
        if (reg_ptr->data_size % reg_ptr->transfer_size != 0) {
            out->fatal(CALL_INFO, -1, "%s: invalid DMA config!\n", dma->getName().c_str());
        }
        
        dma->dma_ctrl_regs.simulator_mem_addr = reg_ptr->simulator_mem_addr;
        dma->dma_ctrl_regs.sst_mem_addr = reg_ptr->sst_mem_addr;
        dma->dma_ctrl_regs.data_size = reg_ptr->data_size;
        dma->dma_ctrl_regs.transfer_size = reg_ptr->transfer_size;
        dma->dma_ctrl_regs.dir = reg_ptr->dir;
        dma->dma_ctrl_regs.status = DMA_BUSY;

        // Free resource
        delete reg_ptr;

        // We will send the response back once DMA transfer is done in `tick` func
    }
}

/**
 * @brief Handle response from copying sst mem space data to simulator mem space
 * 
 * @param read 
 */
void DMAEngine::DMAHandlers::handle(StandardMem::ReadResp* resp) {
    // Find the simulator buffer pointer value by offset
    // of the sst mem space addr
    size_t offset = (dma->dma_ctrl_regs.sst_mem_addr - (resp->pAddr));
    uint8_t * offseted_ptr = dma->dma_ctrl_regs.simulator_mem_addr - offset;

    // Perform copy from response
    for (uint32_t i = 0; i < dma->dma_ctrl_regs.transfer_size; i++) {
        *offseted_ptr = resp->data[i];
        offseted_ptr++;
    }

    // Check if this is the last copy
    if (dma->dma_ctrl_regs.status == DMA_WAITING_DONE) {
        dma->dma_ctrl_regs.status = DMA_DONE;
    }

    delete resp;
}

/**
 * @brief Handle response from copying simulator mem space data to sst mem space 
 * 
 * @param write 
 */
void DMAEngine::DMAHandlers::handle(StandardMem::WriteResp* resp) {
    // Check if this is the last copy
    if (dma->dma_ctrl_regs.status == DMA_WAITING_DONE) {
        dma->dma_ctrl_regs.status = DMA_DONE;
    }

    delete resp;
}