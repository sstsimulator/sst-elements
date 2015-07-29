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

pagedMultiMemory::pagedMultiMemory(Component *comp, Params &params) : DRAMSimMemory(comp, params), pagesInFast(0), lastMin(0) {
    string access       = params.find_string("access_time", "35ns");
    self_link = ctrl->configureSelfLink("Self", access,
                                        new Event::Handler<pagedMultiMemory>(this, &pagedMultiMemory::handleSelfEvent));

    maxFastPages = (unsigned int)params.find_integer("max_fast_pages", 256);
    pageShift = (unsigned int)params.find_integer("page_shift", 12);

    accStatsPrefix = params.find_string("accStatsPrefix", "");
    dumpNum = 0;

    string clock_freq       = params.find_string("quantum", "5ms");
    comp->registerClock(clock_freq, 
                        new Clock::Handler<pagedMultiMemory>(this, 
                                                             &pagedMultiMemory::quantaClock));

    // only applies to access pattern stats
    collectStats = 1;

    // register stats
    fastHits = registerStatistic<uint64_t>("fast_hits","1");
    fastSwaps = registerStatistic<uint64_t>("fast_swaps","1");
    fastAccesses = registerStatistic<uint64_t>("fast_acc","1");
    tPages = registerStatistic<uint64_t>("t_pages","1");
}


bool pagedMultiMemory::issueRequest(MemController::DRAMReq *req){
    uint64_t pageAddr = (req->baseAddr_ + req->amtInProcess_) >> pageShift;
    bool inFast = 0;
    auto &page = pageMap[pageAddr];
    page.record(req, collectStats);
    //page.record(req->baseAddr_ + req->amtInProcess_, req->isWrite_, req->reqEvent_->);
    // if we are hitting it "a lot" (4) see if we can put it in fast
    if ((0 == page.inFast) && (page.touched > 4)) { 
        if (pagesInFast < maxFastPages) {
            // put it in
            page.inFast = 1;
            pagesInFast++;
        } else {
            if (maxFastPages > 0) {
	      if(page.touched > lastMin) {
                // we're full, search for someone to bump
	        lastMin = INT_MAX;
	        const auto endP = pageMap.end();
                for (auto p = pageMap.begin(); p != endP; ++p) {
		  if ((p->second.inFast == 1) && (p->first != pageAddr)) {
		    lastMin = min(lastMin, int(p->second.touched));
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


void pagedMultiMemory::printAccStats() {
  FILE * pFile;
  char buf[100];
  snprintf(buf, 100, "%s-%d.out", accStatsPrefix.c_str(), dumpNum);
  dumpNum++;

  pFile = fopen (buf,"w");
  if (NULL == pFile) {
    printf("Coulnd't open %s for output\n", buf);
    exit(-1);
  } else {

    for (auto p = pageMap.begin(); p != pageMap.end(); ++p) {
      p->second.printAndClearRecord(p->first, pFile);
    }

    fclose(pFile);
  }
}

void pagedMultiMemory::finish(){
    printf("fast_t_pages: %lu\n", pageMap.size());

    printAccStats();

    DRAMSimMemory::finish();
}


void pagedMultiMemory::handleSelfEvent(SST::Event *event){
    MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
    MemController::DRAMReq *req = ev->req;
    ctrl->handleMemResponse(req);
    delete event;
}

bool pagedMultiMemory::quantaClock(SST::Cycle_t _cycle) {
  printAccStats();

    //printf("Quantum\n");
    for (auto p = pageMap.begin(); p != pageMap.end(); ++p) {
        p->second.touched = p->second.touched >> 1;
    }
    return false;
}
