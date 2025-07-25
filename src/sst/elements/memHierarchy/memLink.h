// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MEMLINK_H_
#define _MEMHIERARCHY_MEMLINK_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/link.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/memLinkBase.h"

namespace SST {
namespace MemHierarchy {

/*
 *  MemLink coordinates all traffic through a port
 *  MemNIC is a type of MemLink
 *  A basic MemLink does not implement SimpleNetwork though
 */
class MemLink : public MemLinkBase {

public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(MemLink, "memHierarchy", "MemLink", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Memory-oriented link interface", SST::MemHierarchy::MemLinkBase)

    /* Define params, inherit from base class */
#define MEMLINK_ELI_PARAMS MEMLINKBASE_ELI_PARAMS, \
    { "latency",            "(string) Additional link latency.", "0ps"},\
    { "port",               "(string) Set by parent component. Name of port this memLink sits on.", "port"}

    SST_ELI_DOCUMENT_PARAMS( { MEMLINK_ELI_PARAMS }  )

    SST_ELI_DOCUMENT_PORTS( { "port", "Port to another memory component", {"memHierarchy.MemEventBase"} } )

/* Begin class definition */
    class MemEventLinkInit : public MemEventBase {
        public:
            MemEventLinkInit(std::string src, MemRegion region) : MemEventBase(src, Command::NULLCMD), region(region) { }

            MemRegion getRegion() { return region; }
            void setRegion(MemRegion reg) { region = reg; }

            virtual MemEventLinkInit* clone(void) override {
                return new MemEventLinkInit(*this);
            }

            virtual std::string getVerboseString(int level = 1) override {
                std::ostringstream str;
                str << "Start: " << region.start << " End: " << region.end;
                str << " Step: " << region.interleaveStep << " Size: " << region.interleaveSize;
                return MemEventBase::getVerboseString(level) + str.str();
            }

            virtual std::string getBriefString() override {
                std::ostringstream str;
                str << "Start: " << region.start << " End: " << region.end;
                str << " Step: " << region.interleaveStep << " Size: " << region.interleaveSize;
                return MemEventBase::getBriefString() + str.str();
            }

        private:
            MemRegion region;

    };

    /* Constructor */
    MemLink(ComponentId_t id, Params &params, TimeConverter* tc);

    /* Destructor */
    virtual ~MemLink() { }

    /* Initialization functions for parent */
    virtual void init(unsigned int phase) override;
    virtual void setup() override;

    /* Shutdown functions for parent */
    virtual void complete(unsigned int phase) override;

    /* Remote endpoint info management */
    virtual std::set<EndpointInfo>* getSources() override;
    virtual std::set<EndpointInfo>* getDests() override;
    virtual std::set<EndpointInfo>* getPeers() override;
    virtual bool isDest(std::string str) override;
    virtual bool isSource(std::string str) override;
    virtual bool isPeer(std::string str) override;
    virtual std::string findTargetDestination(Addr addr) override;
    virtual std::string getTargetDestination(Addr addr) override;
    virtual bool isReachable(std::string dst) override;

    /* Send and receive functions for MemLink */
    virtual void sendUntimedData(MemEventInit * ev, bool broadcast, bool lookup_dst) override;
    virtual MemEventInit* recvUntimedData() override;
    virtual void send(MemEventBase * ev) override;
    virtual MemEventBase * recv();

    /* Debug */
    virtual void printStatus(Output &out) override {
        out.output("  MemHierarchy::MemLink: No status given\n");
    }
    virtual std::string getAvailableDestinationsAsString() override;

    void recvNotify(SST::Event * ev) { (*recvHandler)(ev); }

    MemLink() = default;
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::MemHierarchy::MemLink)
protected:
    void addRemote(EndpointInfo info);
    void addEndpoint(EndpointInfo info);

    // Link
    SST::Link* link_;

    // Data structures
    std::set<EndpointInfo> remotes_;            // Tracks remotes (source or dest) immediately accessible on the other side of our link
    std::set<EndpointInfo> peers_;              // Tracks peers immediately accessible on the other side of our link
    std::set<EndpointInfo> endpoints_;          // Tracks endpoints in the system with info on how to get there
    std::set<std::string> reachable_names_;     // Tracks reachable names for faster lookup than iterating via remotes/peers
    std::set<std::string> peer_names_;          // Tracks peer names for faster lookup than iterating via peers

    // For events that require destination names during init
    std::set<MemEventInit*> init_send_queue_;

private:

};

} //namespace memHierarchy
} //namespace SST

#endif
