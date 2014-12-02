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
#include "sst/core/rng/marsaglia.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

using namespace std;
namespace SST {
namespace MemHierarchy {

    
class ReplacementMgr{
    public:
        typedef unsigned int uint;
        virtual void update(uint id) = 0;
        virtual uint getBestCandidate() = 0;
        virtual uint findBestCandidate(uint setBegin, State * state, bool sharersAware) = 0;
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
    uint        numLines;
    uint        numWays;

    struct Rank {
        uint64_t   timestamp;
        uint       sharers;
        uint       owned;
        State      state;

        void reset() {
            state       = I;
            sharers     = 0;
            timestamp   = 0;
            owned       = 0;
        }

        inline bool lessThan(const Rank& other) const {
            if(state == I) return true;
            //if(!CacheArray::CacheLine::inTransition(state) && CacheArray::CacheLine::inTransition(other.state)) return true;
            else{
                if (sharers == 0 && other.sharers > 0) return true;
                else if (sharers > 0 && other.sharers == 0) return false;
                else if (!owned && other.owned) return true;
                else if (owned && !other.owned) return false;
                else return timestamp < other.timestamp;
            }
        }
    };

public:
    LRUReplacementMgr(Output* _dbg, uint _numLines, uint _numWays, bool _sharersAware) : timestamp(1), bestCandidate(-1), numLines(_numLines), numWays(_numWays)  {
        array = (uint64_t*) calloc(numLines, sizeof(uint64_t));
    }

    virtual ~LRUReplacementMgr() {
        free(array);
    }

    void update(uint id) { array[id] = timestamp++; }
    
    uint findBestCandidate(uint setBegin, State * state, bool sharersAware) {
        uint setEnd = setBegin + numWays;
        bestCandidate = setBegin;
        Rank bestRank = {array[setBegin], (sharersAware)? topCC_->numSharers(setBegin) : 0, (sharersAware)? topCC_->ownerExists(setBegin) : 0, state[0]};
        if (state[0] == I) return (uint)bestCandidate;
        setBegin++;
        int i = 1;
        for (uint id = setBegin; id < setEnd; id++) {
            Rank candRank = {array[id], (sharersAware)? topCC_->numSharers(id) : 0, (sharersAware)? topCC_->ownerExists(id) : 0, state[i]};
            if (candRank.lessThan(bestRank)) {
                bestRank = candRank;
                bestCandidate = id;
                if (state[i] == I) return (uint)bestCandidate;
            }
            i++;
        }
        return (uint)bestCandidate;
    }

    uint getBestCandidate() { return (uint)bestCandidate;}

    void replaced(uint id) {
        array[id] = 0;
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
        uint numWays;

        struct Rank {
            LFUInfo lfuInfo;
            uint sharers;
            uint owned;
            State state;

            void reset() {
                state = I;
                sharers = 0;
                owned = 0;
                lfuInfo = (LFUInfo){0, 0};
            }

            inline bool lessThan(const Rank& other, const uint64_t curTs) const {
                if(state == I) return true;
                //else if (valid == other.valid) {
                    if (sharers == 0 && other.sharers > 0) {
                        return true;
                    } else if (sharers > 0 && other.sharers == 0) {
                        return false;
                    } else if (!owned && other.owned) {
                        return true;
                    } else if (owned && !other.owned) {
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

    public:
    
        LFUReplacementMgr(Output* _dbg, uint _numLines, uint _numWays) : timestamp(1), bestCandidate(-1), numLines(_numLines), numWays(_numWays) {
            array = (LFUInfo*)calloc(numLines, sizeof(LFUInfo));
        }

        ~LFUReplacementMgr() {
            free(array);
        }

        void update(uint id) {
            array[id].ts = (array[id].acc*array[id].ts + timestamp)/(array[id].acc + 1);
            array[id].acc++;
            timestamp += 1000;
        }
        uint findBestCandidate(uint setBegin, State * state, bool sharersAware) {
            uint setEnd = setBegin + numWays;
            bestCandidate = setBegin;
            Rank bestRank = {array[setBegin], (sharersAware)? topCC_->numSharers(setBegin) : 0, (sharersAware)? topCC_->ownerExists(setBegin) : 0, state[0] };
            if (state[0] == I) return (uint)bestCandidate; 
        
            setBegin++;
            int i = 1;
            for (uint id = setBegin; id < setEnd; id++) {
                Rank candRank = {array[id], (sharersAware)? topCC_->numSharers(id) : 0, (sharersAware)? topCC_->ownerExists(id) : 0, state[i]};
                if (candRank.lessThan(bestRank, timestamp)) {
                    bestRank = candRank;
                    bestCandidate = id;
                    if (state[i] == I) return (uint)bestCandidate;
                }
                i++;
            }
            return (uint)bestCandidate;
        }

        uint getBestCandidate() { return (uint)bestCandidate; }

        void replaced(uint id) {
            array[id].acc = 0;
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
    uint        numLines;
    uint        numWays;

    struct Rank {
        uint64_t  timestamp;
        uint      sharers;
        uint      owned;
        State     state;

        void reset() {
            state       = I;
            sharers     = 0;
            owned       = 0;
            timestamp   = 0;
        }

        inline bool biggerThan(const Rank& other) const {
            if(state == I) return true;
            else{
                if (sharers == 0 && other.sharers > 0) return true;
                else if (sharers > 0 && other.sharers == 0) return false;
                else if (owned && !other.owned) return false;
                else if (!owned && other.owned) return true;
                else return timestamp > other.timestamp;
            }
        }
    };

public:
    MRUReplacementMgr(Output* _dbg, uint _numLines, uint _numWays, bool _sharersAware) : timestamp(1), bestCandidate(-1), numLines(_numLines), numWays(_numWays)  {
        array = (uint64_t*) calloc(numLines, sizeof(uint64_t));
    }

    virtual ~MRUReplacementMgr() { free(array); }

    void update(uint id) { array[id] = timestamp++; }

    uint findBestCandidate(uint setBegin, State * state, bool sharersAware) {
        uint setEnd = setBegin + numWays;
        bestCandidate = setBegin;
        Rank bestRank = {array[setBegin], (sharersAware)? topCC_->numSharers(setBegin) : 0, (sharersAware)? topCC_->ownerExists(setBegin) : 0, state[0] };
        if (state[0] == I) return (uint)bestCandidate; 
        
        setBegin++;
        int i = 1;
        for (uint id = setBegin; id < setEnd; id++) {
            Rank candRank = {array[id], (sharersAware)? topCC_->numSharers(id) : 0, (sharersAware)? topCC_->ownerExists(id) : 0, state[i]};
            if (candRank.biggerThan(bestRank)) {
                bestRank = candRank;
                bestCandidate = id;
                if (state[i] == I) return (uint)bestCandidate;
            }
            i++;
        }
        return (uint)bestCandidate;
    }


    uint getBestCandidate() { return (uint)bestCandidate;}

    void replaced(uint id) {
        array[id] = 0;
    }

};



/* ------------------------------------------------------------------------------------------
 *  Random
 * ------------------------------------------------------------------------------------------*/
class RandomReplacementMgr : public ReplacementMgr {
 private:
    int32_t bestCandidate;
    uint numWays;
    SST::RNG::MarsagliaRNG randomGenerator_;

public:
    RandomReplacementMgr(Output* _dbg, uint _numWays) : numWays(_numWays), randomGenerator_(1, 1) {
    }
    virtual ~RandomReplacementMgr() {
        
    }

    void update(uint id){}
        
    uint findBestCandidate(uint setBegin, State * state, bool sharersAware) {
        bestCandidate = (randomGenerator_.generateNextUInt32() % numWays) + setBegin;
        return (uint)bestCandidate;
    }

    uint getBestCandidate() {
        return (uint)bestCandidate;
    }

    void replaced(uint id) { }
};

/* ------------------------------------------------------------------------------------------
 *  Not most recently used (nmru)
 * ------------------------------------------------------------------------------------------*/

class NMRUReplacementMgr : public ReplacementMgr {
private:
    int32_t     bestCandidate;
    int32_t *   array;
    uint        numLines;
    uint        numWays;
    SST::RNG::MarsagliaRNG randomGenerator;

public:
    NMRUReplacementMgr(Output* _dbg, uint _numLines, uint _numWays) : bestCandidate(-1), numLines(_numLines), numWays(_numWays), randomGenerator(1,1)  {
        array = (int32_t*) calloc(numLines/numWays, sizeof(int32_t));
    }

    virtual ~NMRUReplacementMgr() { free(array); }

    void update(uint id) { 
        array[id/numWays] = id % numWays; 
    }

    uint findBestCandidate(uint setBegin, State * state, bool sharersAware) {
        int index = randomGenerator.generateNextUInt32() % (numWays-1);
        if (index < array[setBegin/numWays]) bestCandidate = setBegin + index;
        else bestCandidate = setBegin + index + 1;

        return (uint)bestCandidate;
    }


    uint getBestCandidate() { return (uint)bestCandidate;}

    void replaced(uint id) {}

};


}}



#endif	/* REPLACEMENT_PROTOCOL_H */

