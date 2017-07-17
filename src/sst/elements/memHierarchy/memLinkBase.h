// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MEMLINKBASE_H_
#define _MEMHIERARCHY_MEMLINKBASE_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/link.h>
#include <sst/core/subcomponent.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memTypes.h"

namespace SST {
namespace MemHierarchy {

/*
 *  MemLink coordinates all traffic through a port
 *  Ports are address aware
 */
class MemLinkBase : public SST::SubComponent {

public:
    
    // Struct identifying an endpoint
    struct EndpointInfo {
        std::string name;
        uint64_t addr;
        uint32_t id;
        MemRegion region;

        bool operator<(const EndpointInfo &o) const {
            return (region < o.region);
        }
    };
    
    /* Constructor */
    MemLinkBase(Component * comp, Params &params) : SubComponent(comp) {
        /* Create debug output */
        int debugLevel = params.find<int>("debug_level", 0);
        int debugLoc = params.find<int>("debug", 0);
        dbg.init("", debugLevel, 0, (Output::output_location_t)debugLoc);

        // Filter debug by address
        bool found;
        DEBUG_ADDR = params.find<uint64_t>("debug_addr", 0, found);
        DEBUG_ALL = !found;

        // Set up address region
        uint64_t addrStart = params.find<uint64_t>("addr_range_start", 0);
        uint64_t addrEnd = params.find<uint64_t>("addr_range_end", (uint64_t) - 1);
        string ilSize = params.find<std::string>("interleave_size", "0B");
        string ilStep = params.find<std::string>("interleave_step", "0B");

        // Ensure SI units are power-2 not power-10 - for backward compability
        fixByteUnits(ilSize);
        fixByteUnits(ilStep);

        if (!UnitAlgebra(ilSize).hasUnits("B")) {
            dbg.fatal(CALL_INFO, -1, "Invalid param(%s): interleave_size - must be specified in bytes with units (SI units OK). For example, '1KiB'. You specified '%s'\n",
                    getName().c_str(), ilSize.c_str());
        }
        
        if (!UnitAlgebra(ilStep).hasUnits("B")) {
            dbg.fatal(CALL_INFO, -1, "Invalid param(%s): interleave_step - must be specified in bytes with units (SI units OK). For example, '1KiB'. You specified '%s'\n",
                    getName().c_str(), ilSize.c_str());
        }

        info.region.start = addrStart;
        info.region.end = addrEnd;
        info.region.interleaveSize = UnitAlgebra(ilSize).getRoundedValue();
        info.region.interleaveStep = UnitAlgebra(ilStep).getRoundedValue();
        info.name = getName();
        info.addr = 0;
        info.id = 0;

        // Check whether we should accept a region push by someone else
        acceptRegion = params.find<bool>("accept_region", false);
    }
    
    /* Destructor */
    ~MemLinkBase() { }

    /* Initialization functions for parent */
    virtual void setRecvHandler(Event::HandlerBase * handler) { recvHandler = handler; }
    virtual void init(unsigned int phase) { }
    virtual void finish() { }
    virtual void setup() { }

    /* Send and receive functions for MemLink */
    virtual void sendInitData(MemEventInit * ev) =0;
    virtual MemEventInit* recvInitData() =0;
    virtual void send(MemEventBase * ev) =0;
    virtual MemEventBase * recv() =0;

    /* 
     * Extra functions for MemLink derivatives 
     */
    virtual bool clock() { return true; } // No clock
    virtual uint64_t lookupNetworkAddress(const std::string &dst) const { return 0; } // No network address

    // Link call back for incoming events
    void recvNotify(SST::Event * ev) { (*recvHandler)(ev); }

    /* Functions for managing communication according to address */
    virtual std::string findTargetDestination(Addr addr) {
        for (std::set<EndpointInfo>::iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
            if (it->region.contains(addr)) return it->name;
        }

        /* Build error string */
        stringstream error;
        error << getName() + " (MemLinkBase) cannot find a destination for address " << addr << endl;
        error << "Known destination regions: " << endl;
        for (std::set<EndpointInfo>::iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
            error << it->name << " " << it->region.toString() << endl;
        }
        dbg.fatal(CALL_INFO, -1, "%s", error.str().c_str());
        return "";
    }

    virtual bool isRequestAddressValid(Addr addr) { return info.region.contains(addr); }

    /* Functions for managing source/destination information */
    std::set<EndpointInfo> * getSources() { return &sourceEndpointInfo; }
    std::set<EndpointInfo> * getDests() { return &destEndpointInfo; }
    
    void setSources(std::set<EndpointInfo>& srcs) { sourceEndpointInfo = srcs; }
    void setDests(std::set<EndpointInfo>& dsts) { destEndpointInfo = dsts; }
    
    void addSource(EndpointInfo info) { sourceEndpointInfo.insert(info); }
    void addDest(EndpointInfo info) { destEndpointInfo.insert(info); }
    
    virtual bool isDest(std::string str) { return true; } // Anything we get on this link is valid for a dest 
    virtual bool isSource(std::string str) { return true; } // Anything we get on this link is valid for a source

    MemRegion getRegion() { return info.region; }
    MemRegion setRegion(MemRegion region) { info.region = region; }

protected:
    
    // Debug stuff
    Output dbg;
    Addr DEBUG_ADDR;
    bool DEBUG_ALL;

    // Local EndpointInfo
    EndpointInfo info;
    bool acceptRegion; // Accept a region push from a source

    // Other parameters
    std::unordered_set<uint32_t> sourceIDs, destIDs; // IDs which this endpoint cares about

    // Handlers 
    SST::Event::HandlerBase * recvHandler; // Event handler to call when an event is received

    // Data structures
    std::set<EndpointInfo> sourceEndpointInfo;    // endpoint info for each source network endpoint
    std::set<EndpointInfo> destEndpointInfo;      // endpoint info for each destination network endpoint
};

} //namespace memHierarchy
} //namespace SST

#endif
