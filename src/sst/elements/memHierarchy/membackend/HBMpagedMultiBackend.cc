// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Duplicate of the original pagedMultiBackend.h for use
// with the HBM DRAMSim2 model

#include <sst_config.h>

#include <limits>

#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include "sst/core/rng/mersenne.h"
#include "sst/core/timeLord.h" // is this allowed?
#include "sst/elements/memHierarchy/util.h"
#include "membackend/HBMpagedMultiBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace HBMDRAMSim;


HBMpagedMultiMemory::HBMpagedMultiMemory(ComponentId_t id, Params &params)
  : HBMDRAMSimMemory(id, params), pagesInFast(0), lastMin(0) {
    dbg.init("@R:HBMpagedMultiMemory::@p():@l " + getName() + ": ", 0, 0,
             (Output::output_location_t)params.find<int>("debug", 0));
    dbg.output(CALL_INFO, "making HBMpagedMultiMemory controller\n");


    string access = params.find<std::string>("access_time", "35ns");
    self_link = configureSelfLink("Self", access,
                                        new Event::Handler<HBMpagedMultiMemory>(
                                          this, &HBMpagedMultiMemory::handleSelfEvent));

    maxFastPages = params.find<unsigned int>("max_fast_pages", 256);
    pageShift = params.find<unsigned int>("page_shift", 12);

    accStatsPrefix = params.find<std::string>("accStatsPrefix", "");
    dumpNum = 0;

    string clock_freq = params.find<std::string>("quantum", "5ms");
    registerClock(clock_freq,
                        new Clock::Handler<HBMpagedMultiMemory>(this,
                                                             &HBMpagedMultiMemory::quantaClock));

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

    dbg.verbose(CALL_INFO, 1, 0, "Using Mersenne Generator with seed: %" PRIu32 "\n", seed);
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
        HBMDRAMSim::Callback<HBMpagedMultiMemory, void, unsigned int, uint64_t, uint64_t>
            *readDataCB, *writeDataCB;

        readDataCB = new HBMDRAMSim::Callback<HBMpagedMultiMemory, void, unsigned int,
                                           uint64_t, uint64_t>(this, &HBMpagedMultiMemory::dramSimDone);
        writeDataCB = new HBMDRAMSim::Callback<HBMpagedMultiMemory, void, unsigned int,
                                            uint64_t, uint64_t>(this, &HBMpagedMultiMemory::dramSimDone);

        memSystem->RegisterCallbacks(readDataCB, writeDataCB, NULL);
    }
}

// should we add it?
bool HBMpagedMultiMemory::checkAdd(HBMpageInfo &page) {
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

void HBMpagedMultiMemory::do_FIFO_LRU( HBMpageInfo &page, bool &inFast, bool &swapping) {
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
                HBMpageInfo *victimPage = pageList.back();
                HBMpageInfo::pageListIter e = pageList.end();
                bool found = 0;
                while (e != pageList.begin()) {
                    e--;
                    victimPage = *e;
                    if (victimPage->swapDir == HBMpageInfo::NONE) {
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
                    HBMpageInfo::pageListIter le = pageList.end(); le--;
                    page.listEntry = le;
                } else if ((replaceStrat == SCLRU) && (page.scanLeng > scanThreshold)) {
                    // put "scan-y" pages at the back
                    pageList.push_back(&page); // put in back of list
                    HBMpageInfo::pageListIter le = pageList.end(); le--;
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

void HBMpagedMultiMemory::do_LFU( Addr addr, HBMpageInfo &page, bool &inFast, bool &swapping) {
    const uint64_t pageAddr = addr >> pageShift;
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
	        lastMin = std::numeric_limits<uint>::max(); // UINT_MAX;
	        const auto endP = pageMap.end();
                bool found = 0;
                for (auto p = pageMap.begin(); p != endP; ++p) {
		  if ((p->second.inFast == 1) && (p->first != pageAddr)) {
		    lastMin = min(lastMin, p->second.touched);
		    if((p->second.touched < page.touched) &&
                       (p->second.swapDir == HBMpageInfo::NONE)) { // make sure we don't bump someone in motion
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

bool HBMpagedMultiMemory::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned numBytes ){
    uint64_t pageAddr = addr >> pageShift;
    bool inFast = 0;
    bool swapping = 0;
    SimTime_t extraDelay = 0;
    auto &page = pageMap[pageAddr];

    page.record(addr, isWrite, getRequestor(id), collectStats, pageAddr, replaceStrat == LFU8);

    if (maxFastPages > 0) {
        if (modelSwaps && pageIsSwapping(page)) {
            // don't try to swap if we're already swapping that page
            inFast = page.inFast;
        } else {
            if (replaceStrat == LFU || replaceStrat == LFU8) {
                do_LFU( addr, page, inFast, swapping);
            } else {
                do_FIFO_LRU( page, inFast, swapping);
            }
        }
    }

    Req* req = new Req(id,addr,isWrite,numBytes );

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
            return HBMDRAMSimMemory::issueRequest((ReqId)req, addr, isWrite, numBytes);
        }
    }
}

bool HBMpagedMultiMemory::clock(Cycle_t cycle){
    HBMDRAMSimMemory::clock(cycle);

    // put things in the DRAM
    while (!dramQ.empty()) {
        Req *req = dramQ.front();
        bool inserted = HBMDRAMSimMemory::issueRequest((ReqId)req,req->addr,req->isWrite,req->numBytes);
        if (inserted) {
            dramQ.pop();
        } else {
            break;
        }
    }
    return false;
}


void HBMpagedMultiMemory::printAccStats() {
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

void HBMpagedMultiMemory::finish(){
    printf("fast_t_pages: %lu\n", pageMap.size());

    tPages->addData(pageMap.size());

    if (collectStats) printAccStats();

    HBMDRAMSimMemory::finish();
}


void HBMpagedMultiMemory::handleSelfEvent(SST::Event *event){
    MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
    Req *req = ev->req;

    // check if this is a swap read
    auto si = swapToSlow_Reads.find(ev);
    auto si_w = swapToFast_Writes.find(ev);
    if (modelSwaps && si != swapToSlow_Reads.end()) {
        // this event is from the fast mem and should be issued as a
        // read to the slow.
        // issue to dram
        req->isWrite = true;
        //assert(HBMDRAMSimMemory::issueRequest(req));
        queueRequest(req);
        swapToSlow_Writes[req] = si->second;
        swapToSlow_Reads.erase(ev);
        delete ev;
    } else if (modelSwaps && si_w != swapToFast_Writes.end()) {
        // this is from fast mem, indicating a transfer from slow.
        HBMpageInfo *page = si_w->second;
        page->swapsOut -= 1;
	//printf(" got moveToFast write addr:%p ev:%p p:%p sO:%d\n", (void*)(req->baseAddr_ + req->amtInProcess_) ,ev, page, page->swapsOut);
        if (page->swapsOut == 0) {
            swapDone(page, req->addr);
        }
        swapToFast_Writes.erase(si_w);
        delete req;
        delete ev;
    } else {
        // 'normal' event
        handleMemResponse(req->id);
        delete req;
        delete event;
    }
}

bool HBMpagedMultiMemory::quantaClock(SST::Cycle_t _cycle) {
    if (collectStats) printAccStats();

    lastMin = 0;

    for (auto p = pageMap.begin(); p != pageMap.end(); ++p) {
      //p->second.touched = p->second.touched >> 4;
      p->second.touched = 0;
    }
    return false;
}

void HBMpagedMultiMemory::moveToFast(HBMpageInfo &page) {
    assert(page.swapDir == HBMpageInfo::NONE);

    uint64_t addr = page.pageAddr << pageShift;
    const uint numTransfers = 1 << (pageShift - 6); // assume 2^6 byte cache liens

    // mark page as swapping
    page.swapDir = HBMpageInfo::StoF;
    page.swapsOut = numTransfers;

    dbg.debug(_L10_, "moveToFast(%p addr:%p) sO:%d\n", &page, (void*)(addr),
              page.swapsOut);

    // issue reads to slow mem
    for (int i = 0; i < numTransfers; ++i) {
        Req *nreq = new Req(0, addr, false, 64);
	//printf("  -issued to %p\n", (void*)addr);
        //assert(HBMDRAMSimMemory::issueRequest(nreq));
        queueRequest(nreq);
        addr += 64;
        swapToFast_Reads[nreq] = &page; // record that this is a swap
    }
}

void HBMpagedMultiMemory::moveToSlow(HBMpageInfo *page) {
    assert(page->swapDir == HBMpageInfo::NONE);

    uint64_t addr = page->pageAddr << pageShift;
    const uint numTransfers = 1 << (pageShift - 6); // assume 2^6 byte cache liens

    dbg.debug(_L10_, "moveToSlow(%p addr:%p)\n", page, (void*)(addr));

    // mark page as swapping
    page->swapDir = HBMpageInfo::FtoS;
    page->swapsOut = numTransfers;

    // issue reads to fast mem
    for (int i = 0; i < numTransfers; ++i) {
        MemCtrlEvent *ev = new MemCtrlEvent(new Req( 0, addr, false, 64));
        addr += 64;
        self_link->send(1, ev);
        swapToSlow_Reads[ev] = page; // record that this is a swap
    }
}


void HBMpagedMultiMemory::dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle){
    assert(dramReqs.find(addr) != dramReqs.end());
    std::deque<ReqId> &reqs = dramReqs[addr];
    dbg.debug(_L10_, "Memory Request for %" PRIx64 " Finished [%zu reqs]\n", (Addr)addr, reqs.size());
    assert(reqs.size());
    int rs = reqs.size();
    Req* req = (Req*) reqs.front();
    reqs.pop_front();

    if(0 == reqs.size())
        dramReqs.erase(addr);

    auto si = swapToSlow_Writes.find(req);
    auto si_r = swapToFast_Reads.find(req);
    if (modelSwaps && si != swapToSlow_Writes.end()) {
        // this is a returning write from the DRAM
        // mark the page as having less outstanding
        HBMpageInfo *page = si->second;
        page->swapsOut -= 1;
        if (page->swapsOut == 0) {
            swapDone(page, addr);
        }
        swapToSlow_Writes.erase(si);
        delete req;
    } else if (modelSwaps && si_r != swapToFast_Reads.end()) {
        //printf(" got moveToFast read: %p  p:%p sO:%d\n", (void*)(req->baseAddr_+req->amtInProcess_), si_r->second, si_r->second->swapsOut);
        // this is a read returning from the DRAM. Issue a write to fast memory
        req->isWrite = true;
        MemCtrlEvent *ev = new MemCtrlEvent(req);
        self_link->send(1, ev);
        swapToFast_Writes[ev] = si_r->second;
	//printf("  -issued to fast ev:%p\n", ev);
        swapToFast_Reads.erase(si_r);
	//printf("  -swapToFast_reads: %d\n", (int)swapToFast_Reads.size());
    } else {
        // normal request
        assert(req);
        handleMemResponse(req->id);
        delete req;
    }
}

void HBMpagedMultiMemory::swapDone(HBMpageInfo *page, const uint64_t addr) {
    const uint64_t pageAddr = addr >> pageShift;
    dbg.debug(_L10_, "swapDone(%p addr:%p) %d\n", page, (void*)pageAddr, page->swapDir);

    assert(page->swapsOut == 0);
    assert(page->swapDir != HBMpageInfo::NONE);
    assert(&pageMap[pageAddr] == page);


    // launch requests waiting on the swap
    auto &waitList = waitingReqs[pageAddr];
    //printf(" - swapDone releasing %d\n", (int)waitList.size());
    for (auto it = waitList.begin(); it != waitList.end(); ++it) {
        Req *req = *it;
        if (page->swapDir == HBMpageInfo::FtoS) {
            // just finished moving page from fast to slow mem, so issue to DRAM
            //assert(HBMDRAMSimMemory::issueRequest(req));
            queueRequest(req);
        } else {
            // just finished moving page from slow to fast, so issue to fast
            self_link->send(1, new MemCtrlEvent(req));
        }
    }
    waitList.clear();
    waitingReqs.erase(pageAddr);

    // mark page as ready
    page->swapDir = HBMpageInfo::NONE;
}


bool HBMpagedMultiMemory::pageIsSwapping(const HBMpageInfo &page) {
    return (page.swapDir != HBMpageInfo::NONE);
}

