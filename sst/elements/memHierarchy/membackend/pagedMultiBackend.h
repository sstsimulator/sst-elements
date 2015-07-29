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


#ifndef _H_SST_MEMH_PAGEDMULTI_BACKEND
#define _H_SST_MEMH_PAGEDMULTI_BACKEND

#include "membackend/dramSimBackend.h"

#ifdef DEBUG
#define OLD_DEBUG DEBUG
#undef DEBUG
#endif

#include <DRAMSim.h>

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

struct pageInfo {
    uint touched; // how many times it is touched in quanta
    bool inFast;

    uint64_t lastRef;
    typedef enum {LT_NEG_ONE, NEG_ONE, ZERO, ONE, GT_ONE, LAST_CASE} AcCases;
    uint64_t accPat[LAST_CASE];
    set<string> rqstrs; // requestors who have touched this page

    //page.record(req->baseAddr_ + req->amtInProcess_, req->isWrite_, req->reqEvent_->);
  void record(const MemController::DRAMReq *req, bool collectStats) {
    uint64_t addr = req->baseAddr_ + req->amtInProcess_;
    bool isWrite = req->isWrite_;


    if (isWrite) return;
    // record that we've been touched
    touched++;

    if (0 == collectStats) return;

    // note: this is slow, and only works if directory controller is modified to send along the requestor info
    const string &requestor = req->reqEvent_->getRqstr();
    rqstrs.insert(requestor);
    //printf("%s\n", requestor.c_str());

        addr >>= 6; // cacheline
        if (0 == lastRef) {
            // first touch, do nothing
        } else {
            int64_t diff = addr - lastRef;
            if (diff < -1) {
                accPat[LT_NEG_ONE]++;
            } else if (diff == -1) {
                accPat[NEG_ONE]++;
            } else if (diff == 0) {
                accPat[ZERO]++;
            } else if (diff == 1) {
                accPat[ONE]++;
            } else { // (diff >= 1)
                accPat[GT_ONE]++;
            }
        }
        lastRef = addr;
    }

    void printAndClearRecord(uint64_t addr, FILE *outF) {
        uint64_t sum = 0;
        for (int i = 0; i < LAST_CASE; ++i) {
            sum += accPat[i];
        }
	if (sum > 0) {
	  fprintf(outF, "Page: %" PRIu64 " %" PRIu64, addr, sum);
	  for (int i = 0; i < LAST_CASE; ++i) {
	    fprintf(outF, " %.1f", double(accPat[i]*100)/double(sum));
	  }
	  fprintf(outF, " %" PRIu64, rqstrs.size());
	  fprintf(outF, "\n");
	}	  

        //clear
	for (int i = 0; i < LAST_CASE; ++i) {
	  accPat[i] = 0;
	} 
	rqstrs.clear();
    }

    pageInfo() : touched(0), inFast(0), lastRef(0) {
        for (int i = 0; i < LAST_CASE; ++i) {
            accPat[i] = 0;
        }
    }
};

class pagedMultiMemory : public DRAMSimMemory {
public:
    pagedMultiMemory(Component *comp, Params &params);
    virtual bool issueRequest(MemController::DRAMReq *req);
    virtual void clock();
    virtual void finish();

private:
    void printAccStats();
    string accStatsPrefix;
    int dumpNum;

    class MemCtrlEvent : public SST::Event {
    public:
        MemCtrlEvent(MemController::DRAMReq* req) : SST::Event(), req(req)
        { }

        MemController::DRAMReq *req;
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
            ar & BOOST_SERIALIZATION_NVP(req);
        }
    };

    typedef map<uint64_t, pageInfo> pageMap_t;
    pageMap_t pageMap;
    int maxFastPages;
    int pageShift;
    int pagesInFast;
    int lastMin;
    bool collectStats;

    void handleSelfEvent(SST::Event *event);
    bool quantaClock(SST::Cycle_t _cycle);
    Link *self_link;

    // statistics
    Statistic<uint64_t> *fastHits;
    Statistic<uint64_t> *fastSwaps;
    Statistic<uint64_t> *fastAccesses;
    Statistic<uint64_t> *tPages;
};

}
}

#endif
