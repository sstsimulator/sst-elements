// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_CUSTOMCMDMEMHANDLER_H_
#define _MEMHIERARCHY_CUSTOMCMDMEMHANDLER_H_

#include <string>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/stdMem.h>

#include "sst/elements/memHierarchy/memEventBase.h"

namespace SST {
namespace MemHierarchy {

/*
 * Base class handler for custom commands
 * at the memory controller
 */
class CustomCmdMemHandler : public SST::SubComponent {

public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MemHierarchy::CustomCmdMemHandler, std::function<void(Addr,size_t,std::vector<uint8_t>&)>, std::function<void(Addr,std::vector<uint8_t>*)>)

    class MemEventInfo {
    public:
        std::set<Addr> addrs;   /* Cache line address(es) accessed by this instruction */
        bool shootdown;         /* Whether this event should be coherent w.r.t. caches (shootdown needed) */

        /* Constructors */
        MemEventInfo(std::set<Addr> addrs, bool shootdown = false) : addrs(addrs), shootdown(shootdown) { }
        MemEventInfo(Addr addr, bool sdown = false) { addrs.insert(addr); shootdown = sdown; }
        /* Destructor */
        ~MemEventInfo() {};
    };


    /* Constructor */

    CustomCmdMemHandler(ComponentId_t id, Params &params, std::function<void(Addr,size_t,std::vector<uint8_t>&)> read, std::function<void(Addr,std::vector<uint8_t>*)> write) : SubComponent(id) {
        /* Create debug output */
        int debugLevel = params.find<int>("debug_level", 0);
        int debugLoc = params.find<int>("debug", 0);
        dbg.init("", debugLevel, 0, (Output::output_location_t)debugLoc);

        // Filter by debug address - this is the standard memH debug code
        std::vector<uint64_t> addrArray;
        params.find_array<uint64_t>("debug_addr", addrArray);
        for (std::vector<uint64_t>::iterator it = addrArray.begin(); it != addrArray.end(); it++) {
            DEBUG_ADDR.insert(*it);
        }

        // Calls to read & write data
        readData = read;
        writeData = write;
    }

    /* Destructor */
    ~CustomCmdMemHandler() { }

    /*
     * Interface between handler and memController
     */

    /* Receive should decode a custom event and return an OutstandingEvent struct
     * to the memory controller so that it knows how to process the event
     */
    virtual MemEventInfo receive(MemEventBase* ev) =0;

    /* The memController will call ready when the event is ready to issue.
     * Events are ready immediately (back-to-back receive() and ready()) unless
     * the event needs to stall for some coherence action.
     * The handler should return a CustomData* which will be sent to the memBackendConvertor.
     * The memBackendConvertor will then issue the unmodified CustomCmdReq* to the backend.
     * CustomCmdReq is intended as a base class for custom commands to define as needed.
     */
    virtual Interfaces::StandardMem::CustomData* ready(MemEventBase* ev) =0;

    /* When the memBackendConvertor returns a response, the memController will call this function, including
     * the return flags. This function should return a response event or null if none needed.
     * It should also call the following as needed:
     *  writeData(): Update the backing store if this custom command wrote data
     *  readData(): Read the backing store if the response needs data
     *  translateLocalT
     */
    virtual MemEventBase* finish(MemEventBase *ev, uint32_t flags) =0;

protected:

    // Debug
    Output dbg;
    std::set<Addr> DEBUG_ADDR;

    std::function<void(Addr,size_t,std::vector<uint8_t>&)> readData;
    std::function<void(Addr,std::vector<uint8_t>*)> writeData;

};

} //namespace memHierarchy
} //namespace SST

#endif
