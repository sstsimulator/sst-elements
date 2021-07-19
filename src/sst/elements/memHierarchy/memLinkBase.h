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
#include <sst/core/warnmacros.h>

#include "sst/elements/memHierarchy/memEventBase.h"
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

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MemHierarchy::MemLinkBase)

#define MEMLINKBASE_ELI_PARAMS { "debug",              "(int) Where to print debug output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},\
    { "debug_level",        "(int) Debug verbosity level. Between 0 and 10", "0"},\
    { "debug_addr",         "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},\
    { "accept_region",      "(bool) Set by parent component but user should unset if region (addr_range_start/end, interleave_size/step) params are provided to memory. Provides backward compatibility for address translation between memory controller and directory.", "false"}


    // Struct identifying an endpoint
    struct EndpointInfo {
        std::string name;   /* Component name (in SST configuration script) */
        uint64_t addr;      /* Component address */
        uint32_t id;        /* Which memory level or group this component belongs to - for determining which components are sources or destinations */
        MemRegion region;   /* Address region associated with this component */

        bool operator<(const EndpointInfo &o) const {
            if (region != o.region)
                return region < o.region;
            else
                return name < o.name;
        }
    };

    /* Constructor */
    MemLinkBase(ComponentId_t id, Params &params) : SubComponent(id) {
        /* Create debug output */
        int debugLevel = params.find<int>("debug_level", 0);
        int debugLoc = params.find<int>("debug", 0);
        dbg.init("", debugLevel, 0, (Output::output_location_t)debugLoc);

        // Filter debug by address
        std::vector<uint64_t> addrArray;
        params.find_array<uint64_t>("debug_addr", addrArray);
        for (std::vector<uint64_t>::iterator it = addrArray.begin(); it != addrArray.end(); it++) {
            DEBUG_ADDR.insert(*it);
        }

        // Set up address region TODO deprecate in the next major release (SST 10)
        bool found, foundany;
        uint64_t addrStart = params.find<uint64_t>("addr_range_start", 0, found);
        foundany = found;
        uint64_t addrEnd = params.find<uint64_t>("addr_range_end", (uint64_t) - 1, found);
        foundany |= found;
        string ilSize = params.find<std::string>("interleave_size", "0B", found);
        foundany |= found;
        string ilStep = params.find<std::string>("interleave_step", "0B", found);
        foundany |= found;
        if (foundany) {
            dbg.output("%s, Warning: Region parameters given to link managers (addr_range_start/end, interleave_size/step) will be overwritten if the component sets them; specify region via component to eliminate this message\n",
                    getName().c_str());
        }

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
        info.name = getParentComponentName();
        info.addr = 0;
        info.id = 0;

        // Check whether we should accept a region push by someone else
        acceptRegion = params.find<bool>("accept_region", false);
    }

    /* Destructor */
    virtual ~MemLinkBase() { }

    /* Initialization functions for parent */
    virtual void setRecvHandler(Event::HandlerBase * handler) { recvHandler = handler; }
    virtual bool isClocked() { return false; }
    virtual void init(unsigned int UNUSED(phase)) { }
    virtual void finish() { }
    virtual void setup() { }

    /* Debug - triggered by output.fatal() or SIGUSR2 */
    virtual void printStatus(Output &out) {
        out.output("  MemHierarchy::MemLinkBase: No status given\n");
    }

    virtual void emergencyShutdownDebug(Output &out) { }

    /* Send and receive functions for MemLink */
    virtual void sendInitData(MemEventInit * ev) =0;
    virtual MemEventInit* recvInitData() =0;
    virtual void send(MemEventBase * ev) =0;

    /*
     * Extra functions for MemLink derivatives
     */
    virtual bool clock() { return true; } // No clock

    // Link call back for incoming events
    void recvNotify(SST::Event * ev) { (*recvHandler)(ev); }

    /* Functions for managing communication according to address */
    virtual std::string findTargetDestination(Addr addr) =0;

    virtual bool isRequestAddressValid(Addr addr) { return info.region.contains(addr); }

    /* Functions for managing source/destination information */
    virtual std::set<EndpointInfo>* getSources() =0;
    virtual std::set<EndpointInfo>* getDests() =0;

    virtual bool isDest(std::string UNUSED(str)) =0;
    virtual bool isSource(std::string UNUSED(str)) =0;

    MemRegion getRegion() { return info.region; }
    void setRegion(MemRegion region) { info.region = region; }

    EndpointInfo getEndpointInfo() { return info; }
    void setEndpointInfo(EndpointInfo i) { info = i; }

    // Optional. This sets the name used for src/destination/requestor/etc.
    // This should be set correctly by default but occasionally 
    // a specific implementation may require a different setting
    void setName(std::string name) { info.name = name; }

protected:

    // Debug stuff
    Output dbg;
    std::set<Addr> DEBUG_ADDR;

    // Local EndpointInfo
    EndpointInfo info;
    bool acceptRegion; // Accept a region push from a source

    // Handlers
    SST::Event::HandlerBase * recvHandler; // Event handler to call when an event is received

    // Data structures
    std::queue<MemEventInit*> initReceiveQ;     // queue for messages received during init

private:

};

} //namespace memHierarchy
} //namespace SST

#endif
