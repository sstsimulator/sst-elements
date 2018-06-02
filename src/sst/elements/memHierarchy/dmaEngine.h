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

#ifndef _MEMHIERARCHY_DMAENGINE_H_
#define _MEMHIERARCHY_DMAENGINE_H_



#include <vector>

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/elementinfo.h>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memNIC.h"


namespace SST {
namespace MemHierarchy {

/* Send this to the DMAEngine to cause a DMA.  Returned when complete. */
class DMACommand : public Event {
private:
    static uint64_t main_id;
    SST::Event::id_type event_id;
public:
    Addr dst;
    Addr src;
    size_t size;
    DMACommand(const Component *origin, Addr dst, Addr src, size_t size) :
        Event(), dst(dst), src(src), size(size)
    {
      event_id = std::make_pair(main_id++, origin->getId());
    }
    SST::Event::id_type getID(void) const { return event_id; }

private:
    DMACommand() {} // For serialization
};


class DMAEngine : public Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(DMAEngine, "memHierarchy", "DMAEngine", SST_ELI_ELEMENT_VERSION(1,0,0),
            "DMA Engine", COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
            {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
            {"debug_level",     "Debugging level: 0 to 10", "0"},
            {"clockRate",       "Clock Rate for processing DMAs.", "1GHz"},
            {"netAddr",         "Network address of component.", NULL},
            {"network_num_vc",  "DEPRECATED. Number of virtual channels (VCs) on the on-chip network. memHierarchy only uses one VC.", "1"},
            {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE.", "0"} )

    SST_ELI_DOCUMENT_PORTS( {"netLink", "Network Link", {"memHierarchy.MemRtrEvent"} } )

/* Begin class definition */
private:
    struct Request {
        DMACommand *command;
        std::set<SST::Event::id_type> loadKeys;
        std::set<SST::Event::id_type> storeKeys;

        Addr getDst() const { return command->dst; }
        Addr getSrc() const { return command->src; }
        size_t getSize() const { return command->size; }

        Request(DMACommand *cmd) :
            command(cmd)
        { }
    };

    std::deque<DMACommand*> commandQueue;
    std::set<Request*> activeRequests;

    Output dbg;
    uint64_t blocksize;
    Output::output_location_t statsOutputTarget;
    uint64_t numTransfers;
    uint64_t bytesTransferred;


    Link *commandLink;
    MemNIC *networkLink;

public:
    DMAEngine(ComponentId_t id, Params& params);
    ~DMAEngine() {}
    virtual void init(unsigned int phase);
    virtual void setup();
    virtual void finish();

private:
    DMAEngine() {}; // For serialization

    bool clock(Cycle_t cycle);

    bool isIssuable(DMACommand *cmd) const;
    void startRequest(Request *req);
    void processPacket(Request *req, MemEventBase *ev);

    bool findOverlap(DMACommand *c1, DMACommand *c2) const;
    bool findOverlap(Addr a1, size_t s1, Addr a2, size_t s2) const;
    Request* findRequest(SST::Event::id_type id);
    //std::string findTargetDirectory(Addr addr); Moved to MemNIC
};

}
}

#endif

