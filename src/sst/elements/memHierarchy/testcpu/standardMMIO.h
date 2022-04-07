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

#ifndef MEMHIERARCHY_MMIO_EXAMPLE_H
#define MEMHIERARCHY_MMIO_EXAMPLE_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/rng/marsaglia.h>

#include "sst/elements/memHierarchy/memTypes.h"

namespace SST {
namespace MemHierarchy {

/* 
 * This is an example MMIO device that uses the StandardMem interface
 * The device is very simple; it computes the square of integers written to it.
 * READ: return the square of the last written value
 * WRITE: square the value sent. Payloads are interpreted as integers.
 */

class StandardMMIO : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(StandardMMIO, "memHierarchy", "mmioEx", SST_ELI_ELEMENT_VERSION(1,0,0),
        "Example MMIO device for testing interface", COMPONENT_CATEGORY_MEMORY)
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

    StandardMMIO(ComponentId_t id, Params &params);

    virtual void init(unsigned int);
    virtual void setup();

protected:
    ~StandardMMIO() {}

    /* Handle event from MMIO interface */
    void handleEvent(Interfaces::StandardMem::Request* req);
    
    /* Handlers for StandardMem::Request types we handle */
    class mmioHandlers : public Interfaces::StandardMem::RequestHandler {
    public:
        friend class StandardMMIO;

        mmioHandlers(StandardMMIO* mmio, SST::Output* out) : Interfaces::StandardMem::RequestHandler(out), mmio(mmio) {}
        virtual ~mmioHandlers() {}
        virtual void handle(Interfaces::StandardMem::Read* read) override;
        virtual void handle(Interfaces::StandardMem::Write* write) override;
        virtual void handle(Interfaces::StandardMem::ReadResp* resp) override;
        virtual void handle(Interfaces::StandardMem::WriteResp* resp) override;

        void intToData(int32_t num, std::vector<uint8_t>* data);
        int32_t dataToInt(std::vector<uint8_t>* data);

        uint32_t tx_buffer;
        StandardMMIO* mmio;
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

}; // end StandardMMIO
        
}
}


#endif /*  MEMHIERARCHY_MMIO_EXAMPLE_H */
