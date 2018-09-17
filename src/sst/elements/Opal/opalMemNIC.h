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

#ifndef _OPAL_MEMNIC_SUBCOMPONENT_H_
#define _OPAL_MEMNIC_SUBCOMPONENT_H_

#include <sst/core/elementinfo.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>

#include "sst/elements/memHierarchy/memNICBase.h"

namespace SST {
namespace Opal {

/* 
 * NIC for multi-node configurations 
 */
class OpalMemNIC : public SST::MemHierarchy::MemNICBase {

public:
/* Element Library Info */
#define OPAL_MEMNIC_ELI_PARAMS MEMNICBASE_ELI_PARAMS, \
    { "node",    	            "Node number in multinode environment", "0"},\
    { "shared_memory",              "Shared meory enable flag", "0"},\
    { "local_memory_size",          "Local memory size to mask local memory addresses", "0"},\
    { "network_bw",                 "Network bandwidth", "80GiB/s" },\
    { "network_input_buffer_size",  "Size of input buffer", "1KiB" },\
    { "network_output_buffer_size", "Size of output buffer", "1KiB" },\
    { "min_packet_size",            "Size of packet with a payload (e.g., control message size)", "8B"}


    SST_ELI_REGISTER_SUBCOMPONENT(OpalMemNIC, "Opal", "OpalMemNIC", SST_ELI_ELEMENT_VERSION(1,0,0),
            "MemNIC for Opal multi-node configurations", "SST::MemHierarchy::MemLinkBase")

    SST_ELI_DOCUMENT_PARAMS( OPAL_MEMNIC_ELI_PARAMS )

    SST_ELI_DOCUMENT_PORTS( {"port", "Link to network", {"memHierarchy.MemRtrEvent"} } )

/* Begin class definition */    

    /* Constructor */
    OpalMemNIC(Component * comp, Params &params);
    
    /* Destructor */
    ~OpalMemNIC() { }
    
    bool clock();
    void send(MemHierarchy::MemEventBase *ev);

    bool recvNotify(int);

    void init(unsigned int phase);
    void finish() { link_control->finish(); }
    void setup() { link_control->setup(); MemLinkBase::setup(); }
    
    virtual std::string findTargetDestination(MemHierarchy::Addr addr);
    virtual void addDest(MemHierarchy::MemLinkBase::EndpointInfo info);

private:
    bool enable;
    uint64_t localMemSize;

    size_t packetHeaderBytes;
    SST::Interfaces::SimpleNetwork * link_control;
    std::queue<SST::Interfaces::SimpleNetwork::Request*> sendQueue;
};

} //namespace Opal
} //namespace SST

#endif
