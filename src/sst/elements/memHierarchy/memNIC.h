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
#include "sst/elements/memHierarchy/memLinkBase.h"

namespace SST {
namespace MemHierarchy {

/* MemNIC Base class
 *  Base class to handle initialization and endpoint management for different NICs
 */
class MemNICBase : public MemLinkBase {
    public:

#define MEMNICBASE_ELI_PARAMS MEMLINKBASE_ELI_PARAMS, \
        { "group",                       "(int) Group ID. See params 'sources' and 'destinations'. If not specified, the parent component will guess.", "1"},\
        { "sources",                     "(comma-separated list of ints) List of group IDs that serve as sources for this component. If not specified, defaults to 'group - 1'.", "group-1"},\
        { "destinations",                "(comma-separated list of ints) List of group IDs that serve as destinations for this component. If not specified, defaults to 'group + 1'.", "group+1"}

        MemNICBase(Component * comp, Params &params);
        ~MemNICBase() { }

        uint64_t lookupNetworkAddress(const std::string &dst) const;

        // Router events 
        class MemRtrEvent : public SST::Event {
            public:
                MemEventBase * event;
                MemRtrEvent() : Event(), event(nullptr) { }
                MemRtrEvent(MemEventBase * ev) : Event(), event(ev) { }
            
                virtual Event* clone(void) override {
                    MemRtrEvent *mre = new MemRtrEvent(*this);
                    if (this->event != nullptr)
                        mre->event = this->event->clone();
                    else
                        mre->event = nullptr;
                    return mre;
                }

                virtual bool hasClientData() const { return true; }
    
                void serialize_order(SST::Core::Serialization::serializer &ser) override {
                    Event::serialize_order(ser);
                    ser & event;
                }
    
                ImplementSerializable(SST::MemHierarchy::MemNICBase::MemRtrEvent);
        };

        class InitMemRtrEvent : public MemRtrEvent {
            public:
                EndpointInfo info;

                InitMemRtrEvent() {}
                InitMemRtrEvent(EndpointInfo info) : MemRtrEvent(), info(info) { }

                virtual Event* clone(void) override {
                    InitMemRtrEvent * imre = new InitMemRtrEvent(*this);
                    if (this->event != nullptr)
                        imre->event = this->event->clone();
                    else
                        imre->event = nullptr;
                    return imre;
                }

                virtual bool hasClientData() const override { return false; }

                void serialize_order(SST::Core::Serialization::serializer & ser) override {
                    MemRtrEvent::serialize_order(ser);
                    ser & info.name;
                    ser & info.addr;
                    ser & info.id;
                    ser & info.node;
                    ser & info.region.start;
                    ser & info.region.end;
                    ser & info.region.interleaveSize;
                    ser & info.region.interleaveStep;
                }
    
                ImplementSerializable(SST::MemHierarchy::MemNICBase::InitMemRtrEvent);
        };

        // Init functions
        void sendInitData(MemEventInit * ev);
        MemEventInit* recvInitData();

        bool isSource(std::string str) { /* Note this is only used during init so doesn't need to be fast */
            for (std::set<EndpointInfo>::iterator it = sourceEndpointInfo.begin(); it != sourceEndpointInfo.end(); it++) {
                if (it->name == str) return true;   
            }
            return false;
        }

        bool isDest(std::string str) { /* Note this is only used during init so doesn't need to be fast */
            for (std::set<EndpointInfo>::iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
                if (it->name == str) return true;   
            }
            return false;
        }
    
    protected:
        void nicInit(SST::Interfaces::SimpleNetwork * linkcontrol, unsigned int phase);
        bool initMsgSent;

        // Data structures
        std::unordered_map<std::string,uint64_t> networkAddressMap; // Map of name -> address for each network endpoint
        
        // Init queues
        std::queue<MemRtrEvent*> initQueue; // Queue for received init events
        std::queue<SST::Interfaces::SimpleNetwork::Request*> initSendQueue; // Queue of events waiting to be sent after network (linkcontrol) initializes
};
/***** End MemHierarchy::MemNICBase *****/



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
            "Memory-oriented network interface", "SST::MemLinkBase")

    SST_ELI_DOCUMENT_PARAMS( MEMNIC_ELI_PARAMS )

    SST_ELI_DOCUMENT_PORTS( {"port", "Link to network", { "memHierarchy.MemRtrEvent" } } )

/* Begin class definition */    
    /* Constructor */
    MemNIC(Component * comp, Params &params);
    
    /* Destructor */
    ~MemNIC() { }

    /* Functions called by parent for handling events */
    bool clock();
    void send(MemEventBase * ev);
    MemEventBase * recv();
    
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
    void emergencyShutdownDebug(Output &out);

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
