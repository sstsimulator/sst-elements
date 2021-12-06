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
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MemLink, "memHierarchy", "MemLink", SST_ELI_ELEMENT_VERSION(1,0,0),
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

            virtual std::string getVerboseString() override {
                std::ostringstream str;
                str << "Start: " << region.start << " End: " << region.end;
                str << " Step: " << region.interleaveStep << " Size: " << region.interleaveSize;
                return MemEventBase::getVerboseString() + str.str();
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
    virtual void init(unsigned int phase);
    virtual void setup();

    /* Remote endpoint info management */
    virtual std::set<EndpointInfo>* getSources();
    virtual std::set<EndpointInfo>* getDests();
    virtual bool isDest(std::string UNUSED(str));
    virtual bool isSource(std::string UNUSED(str));
    virtual std::string findTargetDestination(Addr addr);
    virtual std::string getTargetDestination(Addr addr);
    virtual bool isReachable(std::string dst);

    /* Send and receive functions for MemLink */
    virtual void sendInitData(MemEventInit * ev, bool broadcast = true);
    virtual MemEventInit* recvInitData();
    virtual void send(MemEventBase * ev);
    virtual MemEventBase * recv();

    /* Debug */
    virtual void printStatus(Output &out) {
        out.output("  MemHierarchy::MemLink: No status given\n");
    }
    virtual std::string getAvailableDestinationsAsString();

protected:
    void addRemote(EndpointInfo info);
    void addEndpoint(EndpointInfo info);

    // Link
    SST::Link* link;

    // Data structures
    std::set<EndpointInfo> remotes;             // Tracks remotes immediately accessible on the other side of our link
    std::set<EndpointInfo> endpoints;           // Tracks endpoints in the system with info on how to get there
    std::set<std::string> remoteNames;          // Tracks remote names for faster lookup than iteratinv via remotes
    
    // For events that require destination names during init
    std::set<MemEventInit*> initSendQ;

private:

};

} //namespace memHierarchy
} //namespace SST

#endif
