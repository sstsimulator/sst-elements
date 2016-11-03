// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_PAGEDMULTI_BACKEND
#define _H_SST_MEMH_PAGEDMULTI_BACKEND

#include <queue>
#include "membackend/dramSimBackend.h"
#include "sst/core/rng/sstrng.h"

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
    typedef list<pageInfo*> pageList_t;
    typedef pageList_t::iterator pageListIter;

    uint64_t pageAddr;
    uint touched; // how many times it is touched in quanta (used in LFU)
    pageListIter listEntry;
    bool inFast;
    SimTime_t lastTouch; // used in mrpuLRU
    uint64_t lastRef; // used in scan detection
    uint scanLeng; // number of consecutive unit-1-stride accesses
    SimTime_t pageDelay; // time when page will be in fast mem

    typedef enum {NONE, FtoS, StoF} swapDir_t;
    swapDir_t swapDir;
    int swapsOut;

    // stats
    typedef enum {LT_NEG_ONE, NEG_ONE, ZERO, ONE, GT_ONE, LAST_CASE} AcCases;
    uint64_t accPat[LAST_CASE];
    set<string> rqstrs; // requestors who have touched this page

    void record( Addr addr, bool isWrite, const std::string& requestor, 
                    const bool collectStats, const uint64_t pAddr, const bool limitTouch) {
        
        // record the pageAddr
        assert((pageAddr == 0) || (pAddr == pageAddr));
        pageAddr = pAddr;

        //stats ignore writes
        if ((1 == collectStats) && isWrite) return;

        // record that we've been touched
        touched++;
	if (limitTouch) {
	  if (touched > 64) touched = 64;
	}

        // detect scans
        addr >>= 6; // cacheline
        if (lastRef != 0) {
            int64_t diff = addr - lastRef;
            if (diff == 1) {
                scanLeng++;
            } else {
                scanLeng = 0;
            }
        }
         
        if (0 == collectStats) {
            lastRef = addr;
            return;
        }
        
        // note: this is slow, and only works if directory controller
        // is modified to send along the requestor info
        if (1 == collectStats) {
            rqstrs.insert(requestor);
            //printf("%s\n", requestor.c_str());
        }

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
	  fprintf(outF, " %" PRIu64, (uint64_t)rqstrs.size());
	  fprintf(outF, "\n");
	}	  

        //clear
	for (int i = 0; i < LAST_CASE; ++i) {
	  accPat[i] = 0;
	} 
	rqstrs.clear();
    }

    pageInfo() : pageAddr(0), touched(0), inFast(0), lastTouch(0), lastRef(0), scanLeng(0),
                 pageDelay(0), swapDir(NONE), swapsOut(0) {
        for (int i = 0; i < LAST_CASE; ++i) {
            accPat[i] = 0;
        }
    }
};

class pagedMultiMemory : public DRAMSimMemory {
public:
    pagedMultiMemory(Component *comp, Params &params);
	virtual bool issueRequest(ReqId, Addr, bool, unsigned );
    virtual void clock();
    virtual void finish();

private:
    Output dbg;
    RNG::SSTRandom*  rng;

	struct Req : public SST::Core::Serialization::serializable {
        Req( ReqId id, Addr addr, bool isWrite, unsigned numBytes ) :
            id(id), addr(addr), isWrite(isWrite), numBytes(numBytes)
        { }
        ReqId id;
        Addr addr;
        bool isWrite;
        unsigned numBytes;
		void serialize_order(SST::Core::Serialization::serializer &ser) {
			ser & id;
			ser & addr;
			ser & isWrite;
			ser & numBytes;
		}
	  private:
        Req() {}
		ImplementSerializable(SST::MemHierarchy::pagedMultiMemory::Req)
    };
    pageInfo::pageList_t pageList; // used in FIFO

    // addition strategy
    typedef enum {addMFU, // Most Frequent
                  addT, // threshold
                  addMRPU, // threshold + most recent previous add
                  addMFRPU, // threshold + most recent previous add
                  addSC, // thresh + scan detection
                  addSCF, // thresh + scan detection
                  addRAND // thresh + random
    } pageAddStrat_t;
    pageAddStrat_t addStrat;
    // replacement / insertion strategy
    typedef enum {LFU, // threshold + MFU addition, LFU replacement
		  LFU8, // 8bit threshold + MFU addition, LFU replacement
                  FIFO, // FIFO replacement
                  LRU, // LRU replacement
                  BiLRU, // bimodal LRU
                  SCLRU, // scan aware
                  LAST_STRAT} pageReplaceStrat_t;
    pageReplaceStrat_t replaceStrat; 

    bool dramBackpressure;

    bool checkAdd(pageInfo &page);
    void do_FIFO_LRU( pageInfo &page, bool &inFast, bool &swapping);
    void do_LFU( Addr, pageInfo &page, bool &inFast, bool &swapping);
    
    void printAccStats();
    queue<Req *> dramQ;
    void queueRequest(Req *r) {
        dramQ.push(r);
    }

    string accStatsPrefix;
    int dumpNum;

    // swap tracking stuff
    const bool modelSwaps = 1;
    map<uint64_t, list<Req*> > waitingReqs;
public:    
    class MemCtrlEvent;
private:    
    typedef map<MemCtrlEvent *, pageInfo*> evToPage_t;
    typedef map<Req *, pageInfo*> reqToPage_t;
    evToPage_t swapToSlow_Reads;
    evToPage_t swapToFast_Writes;
    reqToPage_t swapToSlow_Writes;
    reqToPage_t swapToFast_Reads;

    void dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);
    void swapDone(pageInfo *, uint64_t);
    void moveToFast(pageInfo &);
    void moveToSlow(pageInfo *);
    bool pageIsSwapping(const pageInfo &page);

public:    
    class MemCtrlEvent : public SST::Event {
    public:
        MemCtrlEvent(Req* req) : SST::Event(), req(req)
        { }

        Req* req;

    private:   
        MemCtrlEvent() {} // For Serialization only
        
    public:
        void serialize_order(SST::Core::Serialization::serializer &ser) {
            Event::serialize_order(ser);
            ser & req;  // Cannot serialize pointers unless they are a serializable object
        }
        
        ImplementSerializable(SST::MemHierarchy::pagedMultiMemory::MemCtrlEvent);     
    };

    typedef map<uint64_t, pageInfo> pageMap_t;
    pageMap_t pageMap;
    uint maxFastPages;
    uint pageShift;
    uint pagesInFast;
    uint lastMin;
    uint threshold;
    uint scanThreshold;
    SimTime_t transferDelay;
    SimTime_t minAccTime;
    bool collectStats;

    void handleSelfEvent(SST::Event *event);
    bool quantaClock(SST::Cycle_t _cycle);
    Link *self_link;

    // statistics
    Statistic<uint64_t> *fastHits;
    Statistic<uint64_t> *fastSwaps;
    Statistic<uint64_t> *fastAccesses;
    Statistic<uint64_t> *tPages;
    Statistic<uint64_t> *cantSwapOut;
    Statistic<uint64_t> *swapDelays;
};

}
}

#endif
