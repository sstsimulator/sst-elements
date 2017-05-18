// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
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

#include "memEvent.h"
#include "memNIC.h"


namespace SST {
namespace MemHierarchy {

/* Send this to the DMAEngine to cause a DMA.  Returned when complete. */
class DMACommand : public Event {
private:
    static uint64_t main_id;
    MemEvent::id_type event_id;
public:
    Addr dst;
    Addr src;
    size_t size;
    DMACommand(const Component *origin, Addr dst, Addr src, size_t size) :
        Event(), dst(dst), src(src), size(size)
    {
      event_id = std::make_pair(main_id++, origin->getId());
    }
    MemEvent::id_type getID(void) const { return event_id; }

private:
    DMACommand() {} // For serialization
};


class DMAEngine : public Component {

    struct Request {
        DMACommand *command;
        std::set<MemEvent::id_type> loadKeys;
        std::set<MemEvent::id_type> storeKeys;

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
    void processPacket(Request *req, MemEvent *ev);

    bool findOverlap(DMACommand *c1, DMACommand *c2) const;
    bool findOverlap(Addr a1, size_t s1, Addr a2, size_t s2) const;
    Request* findRequest(MemEvent::id_type id);
    //std::string findTargetDirectory(Addr addr); Moved to MemNIC
};

}
}

#endif

