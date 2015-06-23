// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "membackend/pagedMultiBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

pagedMultiMemory::pagedMultiMemory(Component *comp, Params &params) : DRAMSimMemory(comp, params), pagesInFast(0) {
    string access       = params.find_string("access_time", "35ns");
    self_link = ctrl->configureSelfLink("Self", access,
                                        new Event::Handler<pagedMultiMemory>(this, &pagedMultiMemory::handleSelfEvent));

    maxFastPages = (unsigned int)params.find_integer("max_fast_pages", 256);

    string clock_freq       = params.find_string("quantum", "1ms");
    comp->registerClock(clock_freq, 
                        new Clock::Handler<pagedMultiMemory>(this, 
                                                             &pagedMultiMemory::quantaClock));

    // register stats
    fastHits = registerStatistic<uint64_t>("fast_hits","1");
    fastSwaps = registerStatistic<uint64_t>("fast_swaps","1");
    fastAccesses = registerStatistic<uint64_t>("fast_acc","1");
}


bool pagedMultiMemory::issueRequest(MemController::DRAMReq *req){
    uint64_t pageAddr = (req->baseAddr_ + req->amtInProcess_) >> 12;
    bool inFast = 0;
    auto &page = pageMap[pageAddr];
    page.touched++;
    // if we are hitting it "a lot" (4) see if we can put it in fast
    if ((0 == page.inFast) && (page.touched > 4)) { 
        if (pagesInFast < maxFastPages) {
            // put it in
            page.inFast = 1;
            pagesInFast++;
        } else {
            if (maxFastPages > 0) {
                // we're full, search for someone to bump
                for (auto p = pageMap.begin(); p != pageMap.end(); ++p) {
                    if ((p->first != pageAddr) && (p->second.inFast == 1)) {
                        if(p->second.touched < page.touched) {
                            p->second.inFast = 0; // rm old
                            page.inFast = 1; // add new
                            fastSwaps->addData(1);
                            break;
                        }
                    }
                }
            }
        }
    } else {
        /*if (page.inFast) printf("in fast hit %d\n", pagesInFast);
          else printf("in fast miss\n");*/
        inFast = page.inFast;
    }

    fastAccesses->addData(1);
    if (inFast) {
        fastHits->addData(1);
        self_link->send(1, new MemCtrlEvent(req));
        return true;
    } else {
        return DRAMSimMemory::issueRequest(req);
    }
}



void pagedMultiMemory::clock(){
    DRAMSimMemory::clock();
}


void pagedMultiMemory::finish(){
    DRAMSimMemory::finish();
}


void pagedMultiMemory::handleSelfEvent(SST::Event *event){
    MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
    MemController::DRAMReq *req = ev->req;
    ctrl->handleMemResponse(req);
    delete event;
}

bool pagedMultiMemory::quantaClock(SST::Cycle_t _cycle) {
    //printf("Quantum\n");
    for (auto p = pageMap.begin(); p != pageMap.end(); ++p) {
        p->second.touched = p->second.touched >> 1;
    }
    return false;
}
