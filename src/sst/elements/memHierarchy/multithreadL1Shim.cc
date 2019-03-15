// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright(c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "multithreadL1Shim.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;


MultiThreadL1::MultiThreadL1(ComponentId_t id, Params &params) : Component(id) {
    /* Setup output and debug streams */
    output.init("", 1, 0, Output::STDOUT);
    
    int debugLevel = params.find<int>("debug_level", 0);
    debug.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    
    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++) 
        DEBUG_ADDR.insert(*it);

    /* Setup clock */
    clockHandler = new Clock::Handler<MultiThreadL1>(this, &MultiThreadL1::tick);
    clock = registerClock(params.find<std::string>("clock", "1GHz"), clockHandler);
    clockOn = true;
    timestamp = 0; 


    /* Setup links */
    if (isPortConnected("cache")) {
        cacheLink = configureLink("cache", "50ps", new Event::Handler<MultiThreadL1>(this, &MultiThreadL1::handleResponse));
    } else {
        output.fatal(CALL_INFO, -1, "%s, Error: no connected cache port. Please connect a cache to port 'cache'\n", getName().c_str());
    }

    if (!isPortConnected("thread0")) output.fatal(CALL_INFO, -1, "%s, Error: no connected CPU ports. Please connect a CPU to port 'thread0'.\n", getName().c_str());
    std::string linkname = "thread0";
    int linkid = 0;
    while (isPortConnected(linkname)) {
        SST::Link * link = configureLink(linkname, "50ps", new Event::Handler<MultiThreadL1, unsigned int>(this, &MultiThreadL1::handleRequest, linkid));
        threadLinks.push_back(link);
        linkid++;
        linkname = "thread" + std::to_string(linkid);
    }


    /* Setup throughput limiting */
    requestsPerCycle = params.find<uint64_t>("requests_per_cycle", 0);
    responsesPerCycle = params.find<uint64_t>("responses_per_cycle", 0);
}

MultiThreadL1::~MultiThreadL1() {
    while (requestQueue.size()) {
        delete requestQueue.front();
        requestQueue.pop();
    }
    while (responseQueue.size()) {
        delete responseQueue.front();
        responseQueue.pop();
    }
}

void MultiThreadL1::handleRequest(SST::Event * ev, unsigned int threadid) {
    MemEventBase *event = static_cast<MemEventBase*>(ev);
    if (!clockOn) enableClock();
    threadRequestMap.insert(std::make_pair(event->getID(), threadid));
    requestQueue.push(event);
}

void MultiThreadL1::handleResponse(SST::Event * ev) {
    MemEventBase *event = static_cast<MemEventBase*>(ev);
    if (!clockOn) enableClock();
    responseQueue.push(event);
}

bool MultiThreadL1::tick(SST::Cycle_t cycle) {
    timestamp++;
   
    uint64_t sendcount = (requestsPerCycle == 0) ? requestQueue.size() : requestsPerCycle;
    
    /* Drain request queue */
    while (!requestQueue.empty() && sendcount > 0) {
        cacheLink->send(requestQueue.front());
        requestQueue.pop();
        sendcount--;
    }

    sendcount = (responsesPerCycle == 0) ? responseQueue.size() : responsesPerCycle;
    
    /* Drain response queue */
    while (!responseQueue.empty() && sendcount > 0) {
        MemEventBase * event = responseQueue.front();
        responseQueue.pop();
        
        unsigned int linkid = threadRequestMap.find(event->getResponseToID())->second;
        threadRequestMap.erase(event->getResponseToID());
        threadLinks[linkid]->send(event);
        
        sendcount--;
    }

    /* Turn off clock if queues are empty */
    if (requestQueue.empty() && responseQueue.empty()) {
        clockOn = false;
        return true;
    }
    return false;
}

inline void MultiThreadL1::enableClock() {
    clockOn = true;
    timestamp = reregisterClock(clock, clockHandler);
    timestamp--;
}

/** SST init/finish */
void MultiThreadL1::setup() {}

void MultiThreadL1::finish() {}

/*
 *  Init: 
 *      forward all CPU events to memory hierarchy
 *      Broadcast L1's 'SST::MemHierarchy::MemEvent' event to all CPUs
 *
 */
void MultiThreadL1::init(unsigned int phase) {
    SST::Event * ev;

    // Pass CPU events to memory hierarchy, generally these are memory initialization
    for (int i = 0; i < threadLinks.size(); i++) {
        while ((ev = threadLinks[i]->recvInitData()) != NULL) {
            MemEventInit * memEvent = dynamic_cast<MemEventInit*>(ev);
            if (memEvent) {
                cacheLink->sendInitData(memEvent->clone());
            }
            delete ev;
        }
    }
    
    // Broadcast L1 events to connected CPUs
    while ((ev = cacheLink->recvInitData()) != NULL) {
        MemEventInit * memEvent = dynamic_cast<MemEventInit*>(ev);
        if (memEvent) {
            for (int i = 0; i < threadLinks.size(); i++) {
                threadLinks[i]->sendInitData(memEvent->clone());
            }
        }
        delete ev;
    }
}

