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

#ifndef MEMHIERARCHY_MMIO_EXAMPLE_H
#define MEMHIERARCHY_MMIO_EXAMPLE_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>

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
        {"base_addr",               "(uint) Starting addr mapped to the device", "0"})
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( 
        {"iface", "Interface into memory subsystem", "SST::Experimental::Interfaces::StandardMem"}
    )

    StandardMMIO(ComponentId_t id, Params &params);

    virtual void init(unsigned int);
    virtual void setup();

protected:
    ~StandardMMIO() {}

    /* Handle event from MMIO interface */
    void handleEvent(Experimental::Interfaces::StandardMem::Request* req);
    
    /* Handlers for StandardMem::Request types we handle */
    class mmioHandlers : public Experimental::Interfaces::StandardMem::RequestHandler {
    public:
        friend class StandardMMIO;

        mmioHandlers(StandardMMIO* mmio, SST::Output* out) : Experimental::Interfaces::StandardMem::RequestHandler(out), mmio(mmio) {}
        virtual ~mmioHandlers() {}
        virtual void handle(Experimental::Interfaces::StandardMem::Read* read) override;
        virtual void handle(Experimental::Interfaces::StandardMem::Write* write) override;

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

    Experimental::Interfaces::StandardMem* iface;

}; // end StandardMMIO
        
}
}


#endif /*  MEMHIERARCHY_MMIO_EXAMPLE_H */
