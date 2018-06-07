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

#ifndef _MEMHIERARCHY_MEMLINK_H_
#define _MEMHIERARCHY_MEMLINK_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/link.h>
#include <sst/core/elementinfo.h>

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
            "Memory-oriented link interface", "SST::MemLinkBase")

    /* Define params, inherit from base class */
#define MEMLINK_ELI_PARAMS MEMLINKBASE_ELI_PARAMS, \
    { "latency",            "(string) Link latency. Prefix 'cpulink' for up-link towards CPU or 'memlink' for down-link towards memory", "50ps"},\
    { "port",               "(string) Set by parent component. Name of port this memLink sits on.", ""}

    SST_ELI_DOCUMENT_PARAMS( { MEMLINK_ELI_PARAMS }  )

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
    MemLink(Component * comp, Params &params);
    
    /* Destructor */
    ~MemLink() { }

    /* Initialization functions for parent */
    virtual void init(unsigned int phase);

    /* Send and receive functions for MemLink */
    virtual void sendInitData(MemEventInit * ev);
    virtual MemEventInit* recvInitData();
    virtual void send(MemEventBase * ev);
    virtual MemEventBase * recv();

    /* Debug */
    virtual void printStatus(Output &out) {
        out.output("  MemHierarchy::MemLink: No status given\n");
    }

protected:
    
    // Link
    SST::Link* link;
};

} //namespace memHierarchy
} //namespace SST

#endif
