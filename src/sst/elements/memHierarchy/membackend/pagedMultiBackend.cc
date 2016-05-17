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
#include "sst/core/rng/mersenne.h"
#include "sst/core/timeLord.h" // is this allowed?

using namespace SST;
using namespace SST::MemHierarchy;

pagedMultiMemory::pagedMultiMemory(Component *comp, Params &params) : DRAMSimMemory(comp, params), pagesInFast(0), lastMin(0) {
    dbg.init("@R:pagedMultiMemory::@p():@l " + comp->getName() + ": ", 0, 0, 
             (Output::output_location_t)params.find<int>("debug", 0));
    dbg.output(CALL_INFO, "making pagedMultiMemory controller\n");


    string access = params.find<std::string>("access_time", "35ns");
    self_link = ctrl->configureSelfLink("Self", access,
                                        new Event::Handler<pagedMultiMemory>(this, &pagedMultiMemory::handleSelfEvent));

    maxFastPages = params.find<unsigned int>("max_fast_pages", 256);
    pageShift = params.find<unsigned int>("page_shift", 12);

    accStatsPrefix = params.find<std::string>("accStatsPrefix", "");
    dumpNum = 0;

    string clock_freq = params.find<std::string>("quantum", "5ms");
    comp->registerClock(clock_freq, 
                        new Clock::Handler<pagedMultiMemory>(this, 
                                                             &pagedMultiMemory::quantaClock));

    // determine page replacement / addition strategy
    std::string stratStr = params.find<std::string>("page_replace_strategy", "FIFO");
    if (stratStr == "FIFO") {
        replaceStrat = FIFO;
    } else if (stratStr == "LFU") {
        replaceStrat = LFU;
    } else if (stratStr == "LFU8") {
        replaceStrat = LFU8;
    } else if (stratStr == "LRU") {
        replaceStrat = LRU;
    } else if (stratStr == "BiLRU") {
        replaceStrat = BiLRU;
    } else if (stratStr == "SCLRU") {
        replaceStrat = SCLRU;
    } else {
        dbg.fatal(CALL_INFO, -1, "Invalid page replacement Strategy (page_replace_strategy)\n");
    }

    // determin page addition strategy
    stratStr = params.find<std::string>("page_add_strategy", "T");
    if (stratStr == "MFU") {
        addStrat = addMFU;
    } else if (stratStr == "T") {
        addStrat = addT;
    } else if (stratStr == "MRPU") {
        addStrat = addMRPU;
    } else if (stratStr == "MFRPU") {
        addStrat = addMFRPU;
    } else if (stratStr == "SC") {
        addStrat = addSC;
    } else if (stratStr == "SCF") {
        addStrat = addSCF;
    } else if (stratStr == "RAND") {
        addStrat = addRAND;
    } else {
        dbg.fatal(CALL_INFO, -1, "Invalid page addition Strategy (page_add_strategy)\n");
    }

    if (addStrat == addMFU) {
      if ((replaceStrat != LFU) && (replaceStrat != LFU8)) {
	dbg.fatal(CALL_INFO, -1, "MFU page addition strategy requires LFU page replacement strategy\n");
      }
    }

    dramBackpressure = params.find<bool>("dramBackpressure", 1);    

    threshold = params.find<unsigned int>("threshold", 4);    
    scanThreshold = params.find<unsigned int>("scan_threshold", 6);    

    transferDelay = params.find<unsigned int>("transfer_delay", 250);
    minAccTime = self_link->getDefaultTimeBase()->getFactor() / 
        Simulation::getSimulation()->getTimeLord()->getNano()->getFactor();

    const uint32_t seed = params.find<uint32_t>("seed", 1447);

    output->verbose(CALL_INFO, 1, 0, "Using Mersenne Generator with seed: %" PRIu32 "\n", seed);
    rng = new RNG::MersenneRNG(seed);

    // only applies to access pattern stats
    collectStats = params.find<unsigned int>("collect_stats", 0);

    // register stats
    fastHits = registerStatistic<uint64_t>("fast_hits","1");
    fastSwaps = registerStatistic<uint64_t>("fast_swaps","1");
    fastAccesses = registerStatistic<uint64_t>("fast_acc","1");
    tPages = registerStatistic<uint64_t>("t_pages","1");
    cantSwapOut = registerStatistic<uint64_t>("cant_swap","1");
    swapDelays = registerStatistic<uint64_t>("swap_delays","1");

    if (modelSwaps) {
        // use our own callbacks
        DRAMSim::Callback<pagedMultiMemory, void, unsigned int, uint64_t, uint64_t>
            *readDataCB, *writeDataCB;

        readDataCB = new DRAMSim::Callback<pagedMultiMemory, void, unsigned int,
                                           uint64_t, uint64_t>(this, &pagedMultiMemory::dramSimDone);
        writeDataCB = new DRAMSim::Callback<pagedMultiMemory, void, unsigned int, 
                                            uint64_t, uint64_t>(this, &pagedMultiMemory::dramSimDone);

        memSystem->RegisterCallbacks(readDataCB, writeDataCB, NULL);
    }
}

// should we add it?
bool pagedMultiMemory::checkAdd(pageInfo &page) {
    // only add if the dram isn't too busy
    if (dramBackpressure && dramQ.size() >= 4) return false;


    switch (addStrat) {
    case addT: 
        return (page.touched > threshold); 
        break;
    case addMRPU:
    case addMFRPU:
        {
            // based on threshold and if the most recent previous use is
            // more recent than the least recently used page in fast
            if (pageList.empty()) return (page.lastTouch > threshold); // startup case
            
            SimTime_t myLastTouch = page.lastTouch;
            const auto &victimPage = pageList.back();
            if (myLastTouch > victimPage->lastTouch) {
	      if (addStrat == addMFRPU) {
		// more recent && more frequent
		return (page.touched > threshold) && (page.touched > victimPage->touched); 
	      } else {
                // more recent
                return (page.touched > threshold); 
	      }
            } else {
                return false;
            }
        }
        break;

    case addSCF:
      {
            if (pageList.empty()) return (page.lastTouch > threshold); // startup case
            
            if (page.touched > threshold) {
	        SimTime_t myLastTouch = page.lastTouch;
	        const auto &victimPage = pageList.back();

		if (page.touched > victimPage->touched) {
		  if (page.scanLeng > scanThreshold) {
                    // roughly 1:1000 chance
                    return (rng->generateNextUInt32() & 0x3ff) == 0;
		  } else {
                    return true;
		  }
		} else {
		  return false;
		}
            } else {
                return false;
            }

      }
    case addSC:
        {
            if (page.touched > threshold) {
                if (page.scanLeng > scanThreshold) {
                    // roughly 1:1000 chance
                    return (rng->generateNextUInt32() & 0x3ff) == 0;
                } else {
                    return true;
                }
            } else {
                return false;
            }
        }
        return 0;
    case addRAND:
        if (page.touched > threshold) {
            if (pagesInFast < maxFastPages) { // there is room to spare!
                // roughly 1:1000 chance
                return (rng->generateNextUInt32() & 0x3ff) == 0;
            } else {
                // roughly 1:8000 chance
                return (rng->generateNextUInt32() & 0x1fff) == 0;
            }
        } else {
            return false;
        }
    default: 
        dbg.fatal(CALL_INFO, -1, "Strategy not supported\n");
        return 0;
    }
}

void pagedMultiMemory::do_FIFO_LRU(DRAMReq *req, pageInfo &page, bool &inFast, bool &swapping) {  
    swapping = 0; 
    if (0 == page.inFast) {
        // not in fast
        if (checkAdd(page)) { // we're hitting it "a lot"
            if (pagesInFast < maxFastPages) { // there is room to spare!
                // put it in
                page.inFast = 1;
                pagesInFast++;
                pageList.push_front(&page); // put in FIFO/list
                page.listEntry = pageList.begin();
                swapping = 1;
                if (modelSwaps) {moveToFast(page);}
            } else {
                // kick someone out
                pageInfo *victimPage = pageList.back();
                pageInfo::pageListIter e = pageList.end();
                bool found = 0;
                while (e != pageList.begin()) {
                    e--;
                    victimPage = *e;
                    if (victimPage->swapDir == pageInfo::NONE) {
                        found = 1;
                        break;
                    }
                }
                assert(e == victimPage->listEntry);

                if (!found) {
                    // don't move anything.
                    inFast = 0;
                    swapping = 0;
                    page.lastTouch = getCurrentSimTimeNano(); // for mrpu
                    dbg.debug(_L10_, "no pages to swap out (%d candidates)\n", (int)pageList.size());
                    cantSwapOut->addData(1);
                    return;
                }

                victimPage->inFast = 0;
                victimPage->listEntry = pageList.end();
                pageList.erase(e);
                if (modelSwaps) {moveToSlow(victimPage);}
                
                // put this one in
                page.inFast = 1;
                swapping = 1;
                if (modelSwaps) {moveToFast(page);}
                if ((replaceStrat == BiLRU) && ((rng->generateNextUInt32() & 0x7f) == 0)) { // roughly 1:128 chance
                    pageList.push_back(&page); // put in back of list
                    pageInfo::pageListIter le = pageList.end(); le--;
                    page.listEntry = le;
                } else if ((replaceStrat == SCLRU) && (page.scanLeng > scanThreshold)) {
                    // put "scan-y" pages at the back
                    pageList.push_back(&page); // put in back of list
                    pageInfo::pageListIter le = pageList.end(); le--;
                    page.listEntry = le;
                } else {
                    pageList.push_front(&page); // put in front of FIFO/list
                    page.listEntry = pageList.begin();
                }

                fastSwaps->addData(1);
            }
        } else {
            // not in fast, but not worth bringing in
            inFast = page.inFast;
        }
    } else {
        // already in fast
        if (replaceStrat == LRU || replaceStrat == BiLRU || replaceStrat == SCLRU) {
	  if ((replaceStrat == SCLRU) && (page.scanLeng > scanThreshold) && ((rng->generateNextUInt32() & 0x1ff))) {
	    // leave 'scan-y' pages where they are
	    ;
	  } else {
	    // move to the front of list
	    pageList.erase(page.listEntry);
	    pageList.push_front(&page);
	    page.listEntry = pageList.begin();
	  }
        }

        inFast = page.inFast;
    }
    page.lastTouch = getCurrentSimTimeNano(); // for mrpu       
}

void pagedMultiMemory::do_LFU(DRAMReq *req, pageInfo &page, bool &inFast, bool &swapping) {
    const uint64_t pageAddr = (req->baseAddr_ + req->amtInProcess_) >> pageShift;
    inFast = 0;
    swapping = 0;

    // if we are hitting it "a lot" see if we can put it in fast
    if ((0 == page.inFast) && (page.touched > threshold)) { 
        if (pagesInFast < maxFastPages) {
            // put it in
            page.inFast = 1;
            pagesInFast++;
            swapping = 1;
            if (modelSwaps) {moveToFast(page);}
        } else {
            if (maxFastPages > 0) {
	      if(page.touched > lastMin) {
                // we're full, search for someone to bump
	        lastMin = UINT_MAX;
	        const auto endP = pageMap.end();
                bool found = 0;
                for (auto p = pageMap.begin(); p != endP; ++p) {
		  if ((p->second.inFast == 1) && (p->first != pageAddr)) {
		    lastMin = min(lastMin, p->second.touched);
		    if((p->second.touched < page.touched) &&
                       (p->second.swapDir == pageInfo::NONE)) { // make sure we don't bump someone in motion
                        found = 1;
                        p->second.inFast = 0; // rm old
                        if (modelSwaps) {moveToSlow(&(p->second));}
                        page.inFast = 1; // add new
                        fastSwaps->addData(1);
                        swapping = 1;
                        if (modelSwaps) {moveToFast(page);}
                        break;
		    }
		  }
                } // end for

                if (!found) {
                    // don't move anything.
                    inFast = 0;
                    assert(page.inFast == 0);
                    swapping = 0;
                    page.lastTouch = getCurrentSimTimeNano(); // for mrpu
                    dbg.debug(_L10_, "no pages to swap out (%d candidates)\n", 
                              (int)pageMap.size());
                    cantSwapOut->addData(1);
                    return;
                }
	      } 
            }
        }
    } else {
        inFast = page.inFast;
    }
}

bool pagedMultiMemory::issueRequest(DRAMReq *req){
    uint64_t pageAddr = (req->baseAddr_ + req->amtInProcess_) >> pageShift;
    bool inFast = 0;
    bool swapping = 0;
    SimTime_t extraDelay = 0;
    auto &page = pageMap[pageAddr];

    page.record(req, collectStats, pageAddr, replaceStrat == LFU8);

    if (maxFastPages > 0) {
        if (modelSwaps && pageIsSwapping(page)) {
            // don't try to swap if we're already swapping that page
            inFast = page.inFast;
        } else {
            if (replaceStrat == LFU || replaceStrat == LFU8) {
                do_LFU(req, page, inFast, swapping);
            } else {
                do_FIFO_LRU(req, page, inFast, swapping);
            }
        }
    }

    if (modelSwaps) {
        fastAccesses->addData(1);
        if (pageIsSwapping(page)) {
            // put in queue to be issued when swap completes
 	    swapDelays->addData(1);
            waitingReqs[pageAddr].push_back(req);
        } else {
            if (inFast) {
	        fastHits->addData(1);
                // issue to fast
                self_link->send(1, new MemCtrlEvent(req));
            } else {
                // issue to slow
                //return DRAMSimMemory::issueRequest(req);
                queueRequest(req);
            }
        }
        return true;
    } else {
        if (transferDelay > 0) {
            SimTime_t now = getCurrentSimTimeNano(); 
            if (swapping) {
                page.pageDelay = now + transferDelay;  //delay till page can be used
            }
            if (page.pageDelay > now) {
                extraDelay = page.pageDelay - now;
                extraDelay = max(extraDelay, minAccTime); // make sure it is always at least as slow as the fast mem
            }
        }
        
        fastAccesses->addData(1);
        if (inFast) {
            fastHits->addData(1);
            if (extraDelay > 0) {
                self_link->send(extraDelay, 
                                Simulation::getSimulation()->getTimeLord()->getNano(), 
                                new MemCtrlEvent(req));
            } else {
                self_link->send(1, new MemCtrlEvent(req));
            }
            return true;
        } else {
            return DRAMSimMemory::issueRequest(req);
        }
    }
}



void pagedMultiMemory::clock(){
    DRAMSimMemory::clock();

    // put things in the DRAM 
    while (!dramQ.empty()) {
        DRAMReq *req = dramQ.front();
        bool inserted = DRAMSimMemory::issueRequest(req);
        if (inserted) {
            dramQ.pop();
        } else {
            break;
        }
    }
}


void pagedMultiMemory::printAccStats() {
  FILE * pFile;
  char buf[100];
  snprintf(buf, 100, "%s-%d.out", accStatsPrefix.c_str(), dumpNum);
  dumpNum++;

  pFile = fopen (buf,"w");
  if (NULL == pFile) {
      dbg.fatal(CALL_INFO, -1, "Coulnd't open %s for output\n", buf);
  } else {
      for (auto p = pageMap.begin(); p != pageMap.end(); ++p) {
          p->second.printAndClearRecord(p->first, pFile);
      }
      fclose(pFile);
  }
}

void pagedMultiMemory::finish(){
    printf("fast_t_pages: %lu\n", pageMap.size());
    
    tPages->addData(pageMap.size());

    if (collectStats) printAccStats();

    DRAMSimMemory::finish();
}


void pagedMultiMemory::handleSelfEvent(SST::Event *event){
    MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
    DRAMReq *req = ev->req;

    // check if this is a swap read
    auto si = swapToSlow_Reads.find(ev);
    auto si_w = swapToFast_Writes.find(ev);
    if (modelSwaps && si != swapToSlow_Reads.end()) {
        // this event is from the fast mem and should be issued as a
        // read to the slow.
        // issue to dram
        req->cmd_ = PutM;
        //assert(DRAMSimMemory::issueRequest(req));
        queueRequest(req);
        swapToSlow_Writes[req] = si->second;
        swapToSlow_Reads.erase(ev);
        delete ev;
    } else if (modelSwaps && si_w != swapToFast_Writes.end()) {
        // this is from fast mem, indicating a transfer from slow.
        pageInfo *page = si_w->second;
        page->swapsOut -= 1;
	//printf(" got moveToFast write addr:%p ev:%p p:%p sO:%d\n", (void*)(req->baseAddr_ + req->amtInProcess_) ,ev, page, page->swapsOut);
        if (page->swapsOut == 0) {
            swapDone(page, req->baseAddr_ + req->amtInProcess_);
        }
        swapToFast_Writes.erase(si_w);
        delete req;
        delete ev;
    } else {
        // 'normal' event
        ctrl->handleMemResponse(req);
        delete event;
    }
}

bool pagedMultiMemory::quantaClock(SST::Cycle_t _cycle) {
    if (collectStats) printAccStats();
    
    lastMin = 0;

    for (auto p = pageMap.begin(); p != pageMap.end(); ++p) {
      //p->second.touched = p->second.touched >> 4;
      p->second.touched = 0;
    }
    return false;
}

void pagedMultiMemory::moveToFast(pageInfo &page) {
    assert(page.swapDir == pageInfo::NONE);

    uint64_t addr = page.pageAddr << pageShift;
    const uint numTransfers = 1 << (pageShift - 6); // assume 2^6 byte cache liens

    // mark page as swapping
    page.swapDir = pageInfo::StoF;
    page.swapsOut = numTransfers;   

    dbg.debug(_L10_, "moveToFast(%p addr:%p) sO:%d\n", &page, (void*)(addr), 
              page.swapsOut);

    // issue reads to slow mem
    for (int i = 0; i < numTransfers; ++i) {
        DRAMReq *nreq = new DRAMReq(addr, GetS, 64, 64);
	//printf("  -issued to %p\n", (void*)addr);
        //assert(DRAMSimMemory::issueRequest(nreq));
        queueRequest(nreq);
        addr += 64;
        swapToFast_Reads[nreq] = &page; // record that this is a swap
    }
}

void pagedMultiMemory::moveToSlow(pageInfo *page) {
    assert(page->swapDir == pageInfo::NONE);

    uint64_t addr = page->pageAddr << pageShift;
    const uint numTransfers = 1 << (pageShift - 6); // assume 2^6 byte cache liens

    dbg.debug(_L10_, "moveToSlow(%p addr:%p)\n", page, (void*)(addr));

    // mark page as swapping
    page->swapDir = pageInfo::FtoS;
    page->swapsOut = numTransfers;

    // issue reads to fast mem
    for (int i = 0; i < numTransfers; ++i) {
        MemCtrlEvent *ev = new MemCtrlEvent(new DRAMReq(addr, GetS, 64, 64));
        addr += 64;
        self_link->send(1, ev);
        swapToSlow_Reads[ev] = page; // record that this is a swap
    }
}



void pagedMultiMemory::dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle){
    assert(dramReqs.find(addr) != dramReqs.end());
    std::deque<DRAMReq *> &reqs = dramReqs[addr];
    ctrl->dbg.debug(_L10_, "Memory Request for %" PRIx64 " Finished [%zu reqs]\n", (Addr)addr, reqs.size());
    assert(reqs.size());
    int rs = reqs.size();
    DRAMReq *req = reqs.front();
    reqs.pop_front();
    if(0 == reqs.size())
        dramReqs.erase(addr);

    auto si = swapToSlow_Writes.find(req);
    auto si_r = swapToFast_Reads.find(req);
    if (modelSwaps && si != swapToSlow_Writes.end()) {
        // this is a returning write from the DRAM
        // mark the page as having less outstanding
        pageInfo *page = si->second;
        page->swapsOut -= 1;
        if (page->swapsOut == 0) {
            swapDone(page, addr);
        }
        swapToSlow_Writes.erase(si);
        delete req;
    } else if (modelSwaps && si_r != swapToFast_Reads.end()) {
        //printf(" got moveToFast read: %p  p:%p sO:%d\n", (void*)(req->baseAddr_+req->amtInProcess_), si_r->second, si_r->second->swapsOut);
        // this is a read returning from the DRAM. Issue a write to fast memory
        req->cmd_ = PutM;
        MemCtrlEvent *ev = new MemCtrlEvent(req);
        self_link->send(1, ev);
        swapToFast_Writes[ev] = si_r->second;
	//printf("  -issued to fast ev:%p\n", ev);
        swapToFast_Reads.erase(si_r);
	//printf("  -swapToFast_reads: %d\n", (int)swapToFast_Reads.size());
    } else {
        // normal request
        assert(req);
        assert(ctrl);
        ctrl->handleMemResponse(req);
    }
}

void pagedMultiMemory::swapDone(pageInfo *page, const uint64_t addr) {
    const uint64_t pageAddr = addr >> pageShift;
    dbg.debug(_L10_, "swapDone(%p addr:%p) %d\n", page, (void*)pageAddr, page->swapDir);

    assert(page->swapsOut == 0);
    assert(page->swapDir != pageInfo::NONE);
    assert(&pageMap[pageAddr] == page);


    // launch requests waiting on the swap
    auto &waitList = waitingReqs[pageAddr];
    //printf(" - swapDone releasing %d\n", (int)waitList.size());
    for (auto it = waitList.begin(); it != waitList.end(); ++it) {
        DRAMReq *req = *it;
        if (page->swapDir == pageInfo::FtoS) {
            // just finished moving page from fast to slow mem, so issue to DRAM
            //assert(DRAMSimMemory::issueRequest(req));
            queueRequest(req);
        } else {
            // just finished moving page from slow to fast, so issue to fast
            self_link->send(1, new MemCtrlEvent(req));
        }
    }
    waitList.clear();
    waitingReqs.erase(pageAddr);

    // mark page as ready
    page->swapDir = pageInfo::NONE;
}


bool pagedMultiMemory::pageIsSwapping(const pageInfo &page) {
    return (page.swapDir != pageInfo::NONE);
}

