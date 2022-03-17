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

#ifndef BALAR_MMIO_H
#define BALAR_MMIO_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/rng/marsaglia.h>

#include "sst/elements/memHierarchy/memTypes.h"

namespace SST {
namespace MemHierarchy {

/* 
 * Interface to GPGPU-Sim via MMIO
 */

class BalarMMIO : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(BalarMMIO, "memHierarchy", "mmioBalar", SST_ELI_ELEMENT_VERSION(1,0,0),
        "Balar via MMIO interface", COMPONENT_CATEGORY_MEMORY)
    SST_ELI_DOCUMENT_PARAMS( 
        {"verbose",                 "(uint) Determine how verbose the output from the device is", "0"},
        {"clock",                   "(UnitAlgebra/string) Clock frequency", "1GHz"},
        {"base_addr",               "(uint) Starting addr mapped to the device", "0"},
        {"mem_accesses",            "(uint) Number of memory accesses to do", "0"},
        {"max_addr",                "(uint64) Max memory address for requests issued by this device. Required if mem_accesses > 0.", "0"}
    )
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( 
        {"iface", "Interface into memory subsystem", "SST::Interfaces::StandardMem"}
    )
    SST_ELI_DOCUMENT_STATISTICS(
        {"read_latency", "Latency of reads issued by this device to memory", "simulation time units", 1},
        {"write_latency", "Latency of writes issued by this device to memory", "simulation time units", 1},
    )

    BalarMMIO(ComponentId_t id, Params &params);

    virtual void init(unsigned int);
    virtual void setup();

protected:
    ~BalarMMIO() {}

    /* Handle event from MMIO interface */
    void handleEvent(Interfaces::StandardMem::Request* req);
    
    /* Handlers for StandardMem::Request types we handle */
    class mmioHandlers : public Interfaces::StandardMem::RequestHandler {
    public:
        friend class BalarMMIO;

        mmioHandlers(BalarMMIO* mmio, SST::Output* out) : Interfaces::StandardMem::RequestHandler(out), mmio(mmio) {}
        virtual ~mmioHandlers() {}
        virtual void handle(Interfaces::StandardMem::Read* read) override;
        virtual void handle(Interfaces::StandardMem::Write* write) override;
        virtual void handle(Interfaces::StandardMem::ReadResp* resp) override;
        virtual void handle(Interfaces::StandardMem::WriteResp* resp) override;

        void intToData(int32_t num, std::vector<uint8_t>* data);
        int32_t dataToInt(std::vector<uint8_t>* data);

        uint32_t tx_buffer;
        BalarMMIO* mmio;
    };

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void printStatus(Output &out);
    //virtual void emergencyShutdown();
    
    /* Output */
    Output out;

    Addr mmio_addr;
    mmioHandlers* handlers;

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void emergencyShutdown() {};

private:

    // The squared value
    int32_t squared;

    // If this device also is testing memory accesses, these are used
    uint32_t mem_access;
    SST::RNG::MarsagliaRNG rng;
    Interfaces::StandardMem::Request* createWrite(uint64_t addr);
    Interfaces::StandardMem::Request* createRead(Addr addr);
    std::map<Interfaces::StandardMem::Request::id_t, std::pair<SimTime_t, std::string>> requests;
    virtual bool clockTic( SST::Cycle_t );
    Statistic<uint64_t>* statReadLatency;
    Statistic<uint64_t>* statWriteLatency;
    Addr max_addr;

    // The memH interface into the memory system
    Interfaces::StandardMem* iface;

}; // end BalarMMIO
        
}
}


#endif /*  BALAR_MMIO_H */
