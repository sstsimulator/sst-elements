// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   replacementManager.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef REPLACEMENT_MGR_H
#define	REPLACEMENT_MGR_H

#include <assert.h>
#include "coherenceControllers.h"
#include "MESIBottomCoherenceController.h"
#include "MESITopCoherenceController.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

using namespace std;
namespace SST {
namespace MemHierarchy {

    
class ReplacementMgr{
    public:
        typedef unsigned int uint;
        virtual void update(uint id) = 0;
        virtual void startReplacement()= 0; //many policies don't need it
        virtual void recordCandidate(uint id, bool sharersAware, BCC_MESIState state) = 0;
        virtual uint getBestCandidate() = 0;
        virtual void replaced(uint id) = 0;
        void setTopCC(TopCacheController* cc) {topCC_ = cc;}
        void setBottomCC(MESIBottomCC* cc) {bottomCC_ = cc;}
        virtual ~ReplacementMgr(){}
    protected:
        TopCacheController* topCC_;
        MESIBottomCC* bottomCC_;
};

/* ------------------------------------------------------------------------------------------
 *  LRU
 * ------------------------------------------------------------------------------------------*/
class LRUReplacementMgr : public ReplacementMgr {
private:
    uint64_t    timestamp;
    int32_t     bestCandidate;
    uint64_t*   array;
    uint        numLines_;

    struct Rank {
        uint64_t        timestamp;
        uint            sharers;
        BCC_MESIState   state;

        void reset() {
            state       = I;
            sharers     = 0;
            timestamp   = 0;
        }

        inline bool lessThan(const Rank& other) const {
            if(state == I && other.state != I) return true;
            //if(!CacheArray::CacheLine::inTransition(state) && CacheArray::CacheLine::inTransition(other.state)) return true;
            else{
                if (sharers == 0 && other.sharers > 0) return true;
                else if (sharers > 0 && other.sharers == 0) return false;
                else return timestamp < other.timestamp;
            }
        }
    };

Rank bestRank;

public:
    LRUReplacementMgr(Output* _dbg, uint _numLines, bool _sharersAware) : timestamp(1), bestCandidate(-1), numLines_(_numLines)  {
        array = (uint64_t*) calloc(numLines_, sizeof(uint64_t));
        bestRank.reset();
    }

    virtual ~LRUReplacementMgr() {
        free(array);
    }

    void update(uint id) { array[id] = timestamp++; }

    void recordCandidate(uint id, bool sharersAware, BCC_MESIState state) {
        Rank candRank = {array[id], (sharersAware)? topCC_->numSharers(id) : 0, state};
        if (bestCandidate == -1 || candRank.lessThan(bestRank)) {
            bestRank = candRank;
            bestCandidate = id;
        }
        
    }

    uint getBestCandidate() {
        assert(bestCandidate != -1);
        return (uint)bestCandidate;
    }

    void replaced(uint id) {
        bestCandidate = -1;
        bestRank.reset();
        array[id] = 0;
    }

    void startReplacement() {
        bestCandidate = -1;
        bestRank.reset();
    }

};

/* ------------------------------------------------------------------------------------------
 *  LFU
 * ------------------------------------------------------------------------------------------*/
class LFUReplacementMgr : public ReplacementMgr {
    private:
        uint64_t timestamp;
        int32_t bestCandidate;
        struct LFUInfo {
            uint64_t ts;
            uint64_t acc;
        };
        LFUInfo* array;
        uint numLines;

        struct Rank {
            LFUInfo lfuInfo;
            uint sharers;
            BCC_MESIState state;

            void reset() {
                state = I;
                sharers = 0;
                lfuInfo = (LFUInfo){0, 0};
            }

            inline bool lessThan(const Rank& other, const uint64_t curTs) const {
                if(state == I && other.state != I) return true;
                //else if (valid == other.valid) {
                    if (sharers == 0 && other.sharers > 0) {
                        return true;
                    } else if (sharers > 0 && other.sharers == 0) {
                        return false;
                    } else {
                        if (lfuInfo.acc == 0) return true;
                        if (other.lfuInfo.acc == 0) return false;
                        uint64_t ownInvFreq = (curTs - lfuInfo.ts)/lfuInfo.acc; //inverse frequency, lower is better
                        uint64_t otherInvFreq = (curTs - other.lfuInfo.ts)/other.lfuInfo.acc;
                        return ownInvFreq > otherInvFreq;
                    }
               // }
                return false;
            }
           
        };

        Rank bestRank;

    public:
    
        LFUReplacementMgr(Output* _dbg, uint _numLines) : timestamp(1), bestCandidate(-1), numLines(_numLines) {
            array = (LFUInfo*)calloc(numLines, sizeof(LFUInfo));
            bestRank.reset();
        }

        ~LFUReplacementMgr() {
            free(array);
        }

        void update(uint id) {
            array[id].ts = (array[id].acc*array[id].ts + timestamp)/(array[id].acc + 1);
            array[id].acc++;
            timestamp += 1000;
        }

        void recordCandidate(uint id, bool sharersAware, BCC_MESIState state) {
            Rank candRank = {array[id], 0, state};

            if (bestCandidate == -1 || candRank.lessThan(bestRank, timestamp)) {
                bestRank = candRank;
                bestCandidate = id;
            }
        }

        uint getBestCandidate() {
            assert(bestCandidate != -1);
            return (uint)bestCandidate;
        }

        void replaced(uint id) {
            bestCandidate = -1;
            bestRank.reset();
            array[id].acc = 0;
        }
    
        void startReplacement(){
            bestCandidate = -1;
            bestRank.reset();
        }
     
};



/* ------------------------------------------------------------------------------------------
 *  MRU
 * ------------------------------------------------------------------------------------------*/
class MRUReplacementMgr : public ReplacementMgr {
private:
    uint64_t    timestamp;
    int32_t     bestCandidate;
    uint64_t*   array;
    uint        numLines_;

    struct Rank {
        uint64_t      timestamp;
        uint          sharers;
        BCC_MESIState state;

        void reset() {
            state       = I;
            sharers     = 0;
            timestamp   = 0;
        }

        inline bool biggerThan(const Rank& other) const {
            if(state == I && other.state != I) return true;
            else{
                if (sharers == 0 && other.sharers > 0) return true;
                else if (sharers > 0 && other.sharers == 0) return false;
                else return timestamp > other.timestamp;
            }
        }
    };

Rank bestRank;

public:
    MRUReplacementMgr(Output* _dbg, uint _numLines, bool _sharersAware) : timestamp(1), bestCandidate(-1), numLines_(_numLines)  {
        array = (uint64_t*) calloc(numLines_, sizeof(uint64_t));
        bestRank.reset();
    }

    virtual ~MRUReplacementMgr() { free(array); }

    void update(uint id) { array[id] = timestamp++; }

    void recordCandidate(uint id, bool sharersAware, BCC_MESIState state) {
        Rank candRank = {array[id], (sharersAware) ? topCC_->numSharers(id) : 0, state};
        if (bestCandidate == -1 || candRank.biggerThan(bestRank)) {
            bestRank = candRank;
            bestCandidate = id;
        }
        
    }

    uint getBestCandidate() {
        assert(bestCandidate != -1);
        return (uint)bestCandidate;
    }

    void replaced(uint id) {
        bestCandidate = -1;
        bestRank.reset();
        array[id] = 0;
    }

    void startReplacement() {
        bestCandidate = -1;
        bestRank.reset();
    }

};



/* ------------------------------------------------------------------------------------------
 *  Random
 * ------------------------------------------------------------------------------------------*/
class RandomReplacementMgr : public ReplacementMgr {
 private:
    vector<uint64_t> candidates;
    int numWays_;

public:
    RandomReplacementMgr(Output* _dbg, uint _numWays) : numWays_(_numWays) {
        srand (time(NULL));
    }
    virtual ~RandomReplacementMgr() {}

    void update(uint id){}
    void recordCandidate(uint id, bool sharersAware, BCC_MESIState state) { candidates.push_back(id);}

    uint getBestCandidate() {
        int bestCandidate = rand() % numWays_;
        return (uint)candidates[bestCandidate];
    }

    void replaced(uint id) { candidates.clear(); }
    void startReplacement() { candidates.clear(); }
};

}}



#endif	/* REPLACEMENT_PROTOCOL_H */

