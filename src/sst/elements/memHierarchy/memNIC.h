// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MEMNIC_SUBCOMPONENT_H_
#define _MEMHIERARCHY_MEMNIC_SUBCOMPONENT_H_

#include <string>
#include <unordered_map>
#include <queue>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/elementinfo.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memNICBase.h"

namespace SST {
namespace MemHierarchy {

/*
 *  MemNIC provides a simpleNetwork (from SST core) network interface for memory components
 *  and overlays memory functions on top
 *
 *  The memNIC assumes each network endpoint is associated with a set of memory addresses and that
 *  each endpoint communicates with a subset of endpoints on the network as defined by "sources"
 *  and "destinations".
 *
 *  MemNICBase handles init and managing the endpoint information/address lookup
 *
 */
class MemNIC : public MemNICBase {

public:
/* Element Library Info */
#define MEMNIC_ELI_PARAMS MEMNICBASE_ELI_PARAMS, \
        { "network_bw",                  "(string) Network bandwidth", "80GiB/s" },\
        { "network_input_buffer_size",   "(string) Size of input buffer", "1KiB"},\
        { "network_output_buffer_size",  "(string) Size of output buffer", "1KiB"},\
        { "min_packet_size",             "(string) Size of a packet without a payload (e.g., control message size)", "8B"},\
        { "port",                        "(string) Set by parent component. Name of port this NIC sits on.", ""}

    
    SST_ELI_REGISTER_SUBCOMPONENT(MemNIC, "memHierarchy", "MemNIC", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Memory-oriented network interface", "SST::MemHierarchy::MemLinkBase")

    SST_ELI_DOCUMENT_PARAMS( MEMNIC_ELI_PARAMS )

#define MEMNIC_ELI_PORTS {"port", "Link to network", { "memHierarchy.MemRtrEvent" } }

    SST_ELI_DOCUMENT_PORTS( MEMNIC_ELI_PORTS )

/* Begin class definition */    
    /* Constructor */
    MemNIC(Component * comp, Params &params);
    
    /* Destructor */
    ~MemNIC() { }

    /* Functions called by parent for handling events */
    bool clock();
    void send(MemEventBase * ev);
    
    /* Callback to notify when link_control receives a message */
    bool recvNotify(int);

    /* Helper functions */
    size_t getSizeInBits(MemEventBase * ev);

    /* Initialization and finish */
    void init(unsigned int phase);
    void finish() { link_control->finish(); }
    void setup() { link_control->setup(); MemLinkBase::setup(); }

    /* Debug */
    void printStatus(Output &out);

private:

    // Other parameters
    size_t packetHeaderBytes;

    // Handlers and network
    SST::Interfaces::SimpleNetwork *link_control;

    // Event queues
    std::queue<SST::Interfaces::SimpleNetwork::Request*> sendQueue; // Queue of events waiting to be sent (sent on clock)
};

} //namespace memHierarchy
} //namespace SST

#endif
