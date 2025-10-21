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

#ifndef SST_MEMHIERARCHY_BUS_H
#define SST_MEMHIERARCHY_BUS_H

#include <set>
#include <vector>
#include <string>
#include <cstdint>
#include <queue>
#include <map>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/util.h"

using namespace std;

namespace SST { namespace MemHierarchy {

class BusEvent;

/*
 *  Component: memHierarchy.Bus
 *
 *  Connects one or more upper level components to one or more lower level components
 *  over a bus like interface
 */

class Bus : public SST::Component {
public:

/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(Bus, "memHierarchy", "Bus", SST_ELI_ELEMENT_VERSION(1,0,0), "Bus model for memHierarchy", COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
            {"bus_frequency",       "(string) Bus clock frequency"},
            {"broadcast",           "(bool) If set, messages are broadcast to all other ports", "0"},
            {"idle_max",            "(uint) Bus temporarily turns off clock after this number of idle cycles", "6"},
            {"drain_bus",           "(bool) Drain bus on every cycle", "0"},
            {"debug",               "(uint) Output location for debug statements. Requires core configuration flag '--enable-debug'. --0[None], 1[STDOUT], 2[STDERR], 3[FILE]--", "0"},
            {"debug_level",         "(uint) Debugging level: 0 to 10", "0"},
            {"debug_addr",          "(comma separated uints) Address(es) to be debugged. Leave empty for all, otherwise specify one or more comma separated values. Start and end string with brackets", ""} )

    SST_ELI_DOCUMENT_PORTS(
            {"low_network_%(low_network_ports)d", "DEPRECATED. Use 'lowlink\%d' instead. Ports connected to lower level caches (closer to main memory)", {"memHierarchy.MemEventBase"} },
            {"high_network_%(high_network_ports)d", "DEPRECATED. Use 'highlink\%d' instead. Ports connected to higher level caches (closer to processor)", {"memHierarchy.MemEventBase"} },
            {"lowlink%(lowlink_ports)d", "Ports connected to components on the lower/memory side of the bus (i.e., lower level caches, directories, memory, etc.)", {"memHierarchy.MemEventBase"} },
            {"highlink%(highlink_ports)d", "Ports connected to components on the upper/processor side of the bus (i.e., upper level caches, processors, etc.)", {"memHierarchy.MemEventBase"} } )

/* Class definition */

    Bus(SST::ComponentId_t id, SST::Params& params);
    virtual void init(unsigned int phase) override;
    virtual void complete(unsigned int phase) override;

    Bus() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::MemHierarchy::Bus);

private:

    /** Adds event to the incoming event queue.  Reregisters clock if needed */
    void processIncomingEvent(SST::Event *ev);

    /** Send event to a single destination */
    void sendSingleEvent(SST::Event *ev);

    /** Broadcast event to all ports */
    void broadcastEvent(SST::Event *ev);

    /**  Clock Handler */
    bool clockTick(Cycle_t);

    /** Configure Bus objects with the appropriate parameters */
    void configureParameters(SST::Params&);
    void configureLinks();

    void mapNodeEntry(const std::string&, SST::Link*);
    SST::Link* lookupNode(const std::string&);


    Output                      dbg_;
    std::set<Addr>              debug_addr_filter_;
    int                         numHighPorts_;
    int                         numLowPorts_;
    uint64_t                    idleCount_;
    uint64_t                    idleMax_;
    bool                        broadcast_;
    bool                        busOn_;
    bool                        drain_;
    Clock::HandlerBase*         clockHandler_;
    TimeConverter               defaultTimeBase_;

    std::vector<SST::Link*>     highNetPorts_;
    std::vector<SST::Link*>     lowNetPorts_;
    std::map<string,SST::Link*> nameMap_;
    std::queue<SST::Event*>     eventQueue_;

};

}}
#endif /* SST_MEMHIERARHCY__BUS_H */
