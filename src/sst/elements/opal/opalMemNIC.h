// Copyright 2013-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _OPAL_MEMNIC_SUBCOMPONENT_H_
#define _OPAL_MEMNIC_SUBCOMPONENT_H_

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
            "MemNIC for Opal multi-node configurations", SST::MemHierarchy::MemLinkBase)

    SST_ELI_DOCUMENT_PARAMS( OPAL_MEMNIC_ELI_PARAMS )

    SST_ELI_DOCUMENT_PORTS( {"port", "Link to network", {"memHierarchy.MemRtrEvent"} } )

/* Begin class definition */

    /* Constructor */
    OpalMemNIC(ComponentId_t id, Params &params, TimeConverter tc);

    /* Destructor */
    virtual ~OpalMemNIC() { }

    /* Specialized init mem rtr event to specify node */
    class OpalInitMemRtrEvent : public MemHierarchy::MemNICBase::InitMemRtrEvent {
        public:
            uint32_t node;

            OpalInitMemRtrEvent() {}
            OpalInitMemRtrEvent(EndpointInfo info, uint32_t node) : InitMemRtrEvent(info), node(node) { }

            Event* clone(void) override {
                OpalInitMemRtrEvent * imre = new OpalInitMemRtrEvent(*this);
                if (this->event != nullptr)
                    imre->event = this->event->clone();
                else
                    imre->event = nullptr;
                return imre;
            }

            bool hasClientData() const override { return false; }

            void serialize_order(SST::Core::Serialization::serializer &ser) override {
                InitMemRtrEvent::serialize_order(ser);
                SST_SER(node);
            }

            ImplementSerializable(SST::Opal::OpalMemNIC::OpalInitMemRtrEvent);
    };

    bool clock() override;
    void send(MemHierarchy::MemEventBase *ev) override;

    bool recvNotify(int);

    void init(unsigned int phase) override;
    void finish() override { link_control->finish(); }
    void setup() override { link_control->setup(); MemLinkBase::setup(); }

    std::string findTargetDestination(MemHierarchy::Addr addr) override;

    void sendUntimedData(MemHierarchy::MemEventInit* ev, bool broadcast, bool lookup_dst) override;

protected:
    MemHierarchy::MemNICBase::InitMemRtrEvent* createInitMemRtrEvent() override;
    void processInitMemRtrEvent(MemHierarchy::MemNICBase::InitMemRtrEvent* ev) override;

private:
    bool enable;
    uint64_t localMemSize;
    uint32_t node;

    size_t packetHeaderBytes;
    SST::Interfaces::SimpleNetwork * link_control;
    std::queue<SST::Interfaces::SimpleNetwork::Request*> sendQueue;
};

} //namespace Opal
} //namespace SST

#endif
