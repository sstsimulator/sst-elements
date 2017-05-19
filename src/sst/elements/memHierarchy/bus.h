// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_MEMHIERARCHY_BUS_H
#define SST_MEMHIERARCHY_BUS_H

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

class Bus : public SST::Component {
public:

    typedef MemEvent::id_type key_t;
    static const key_t ANY_KEY;
    static const char BUS_INFO_STR[];
    
	Bus(SST::ComponentId_t id, SST::Params& params);
	virtual void init(unsigned int phase);

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
    
    void mapNodeEntry(const std::string&, LinkId_t);
    LinkId_t lookupNode(const std::string&);


    Output                          dbg_;
    bool                            DEBUG_ALL;
    Addr                            DEBUG_ADDR;
    int                             numHighNetPorts_,
                                    numLowNetPorts_,
//                                    numHighNetPortsX_,
//                                    numLowNetPortsX_,
                                    broadcast_,
                                    latency_,
                                    fanout_,
                                    idleMax_,
                                    idleCount_;
    bool                            busOn_;
    Clock::Handler<Bus>*            clockHandler_;
    TimeConverter*                  defaultTimeBase_;
    
    std::string                     busFrequency_;
    std::string                     bus_latency_cycles_;
	std::vector<SST::Link*>         highNetPorts_;
	std::vector<SST::Link*>         lowNetPorts_;
	std::map<string, LinkId_t>      nameMap_;
    std::map<LinkId_t, SST::Link*>  linkIdMap_;
    std::queue<SST::Event*>         eventQueue_;
    
};

}}
#endif /* SST_MEMHIERARHCY__BUS_H */
