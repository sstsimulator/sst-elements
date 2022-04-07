// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_REPLACEMENT_POLICY_H
#define	MEMHIERARCHY_REPLACEMENT_POLICY_H

#include "sst/core/subcomponent.h"
#include "sst/core/rng/marsaglia.h"

#include "memEvent.h"

using namespace std;

namespace SST {
namespace MemHierarchy {

/*
 * Generic/extendable class for passing information between a cache line & a replacement policy
 * To date the coherence policies in memHierarchy only use cache line state and sometimes owned/shared information
 * but other components are able to extend this to pass arbitrary information as needed
 */
class ReplacementInfo {
    public:
        ReplacementInfo(unsigned int i, State s) : index(i), state(s) { }
        virtual ~ReplacementInfo() { }

        unsigned int getIndex() { return index; }
        void setIndex(unsigned int i) { index = i; }

        State getState() { return state; }
        void setState(State s) { state = s; }

    protected:
        unsigned int index;
        State state;
};

class CoherenceReplacementInfo : public ReplacementInfo {
    public:
        CoherenceReplacementInfo(unsigned int i, State s, bool sh, bool o) : shared(sh), owned(o), ReplacementInfo(i, s) { }
        virtual ~CoherenceReplacementInfo() { }

        bool getOwned() { return owned; }
        bool getShared() { return shared; }
        void setOwned(bool o) { owned = o; }
        void setShared(bool s) { shared = s; }
    protected:
        bool owned;
        bool shared;
};


class ReplacementPolicy : public SubComponent{
    public:
        SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MemHierarchy::ReplacementPolicy, uint64_t, uint64_t)

        ReplacementPolicy(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : SubComponent(id) {
        }
        virtual ~ReplacementPolicy(){}

        /* Since we don't dynamic cast ReplacementInfo, do a check here to make sure the type provided by the cache line & the type the replacement policy expects are compatible */
        virtual bool checkCompatibility(ReplacementInfo * rInfo) = 0;

        // Update state
        virtual void update(uint64_t id, ReplacementInfo * rInfo) = 0;
        virtual void replaced(uint64_t id) = 0;

        // Get replacement candidates
        virtual uint64_t getBestCandidate() = 0;
        virtual uint64_t findBestCandidate(std::vector<ReplacementInfo*> &rInfo) = 0;
};

/* ------------------------------------------------------------------------------------------
 *  LRU
 * ------------------------------------------------------------------------------------------*/
class LRU : public ReplacementPolicy {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(LRU, "memHierarchy", "replacement.lru", SST_ELI_ELEMENT_VERSION(1,0,0),
            "least-recently-used replacement policy", SST::MemHierarchy::ReplacementPolicy);


    LRU(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), timestamp(1), bestCandidate(0) {
        ways = associativity;
        array.resize(lines, 0);
    }

    virtual ~LRU() {}

    /* Too expensive to constantly dynamic_cast. Check once during construction instead. */
    bool checkCompatibility(ReplacementInfo * rInfo) { return true; } // No cast

    /* Record recently used line */
    void update(uint64_t id, ReplacementInfo * rInfo) { array[id] = timestamp++; }

    /* Record replaced line */
    void replaced(uint64_t id) { array[id] = 0; }

    /** Lines are selected for replacement according to the following criteria (and in this order):
     * 1. If invalid (alwasy replace these)
     * 2. If owned, try to keep
     * 3. If shared, try to keep
     * 4. If timestamp is the oldest (smallest), then evict
     */
    uint64_t findBestCandidate(std::vector<ReplacementInfo*> &rInfo) {
        bestCandidate = rInfo[0]->getIndex();
        uint64_t bestTS = array[rInfo[0]->getIndex()];
        if (rInfo[0]->getState() == I) {
            return bestCandidate;
        }
        for (int i = 1; i < rInfo.size(); i++) {
            if (rInfo[i]->getState() == I) {
                bestCandidate = rInfo[i]->getIndex();
                return bestCandidate;
            }
            uint64_t candTS = array[rInfo[i]->getIndex()];
            if (candTS < bestTS) {
                bestTS = candTS;
                bestCandidate = rInfo[i]->getIndex();
            }
        }
        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate; }

private:
    uint64_t timestamp;
    uint64_t bestCandidate;
    uint64_t ways;

    std::vector<uint64_t> array;

};


class LRUOpt : public ReplacementPolicy {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(LRUOpt, "memHierarchy", "replacement.lru-opt", SST_ELI_ELEMENT_VERSION(1,0,0),
            "least-recently-used replacement policy with consideration for coherence state", SST::MemHierarchy::ReplacementPolicy);


    LRUOpt(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), timestamp(1), bestCandidate(0) {
        ways = associativity;
        array.resize(lines, 0);
    }

    virtual ~LRUOpt() {}

    /* Too expensive to constantly dynamic_cast. Check once during construction instead. */
    bool checkCompatibility(ReplacementInfo * rInfo) {
        if (dynamic_cast<CoherenceReplacementInfo*>(rInfo))
            return true;
        return false;
    }

    /* Record recently used line */
    void update(uint64_t id, ReplacementInfo * rInfo) { array[id] = timestamp++; }

    /* Record replaced line */
    //void replaced(uint64_t id) { array[id] = 0; }
    void replaced(uint64_t id) { array[id] = 0; }

    /** Lines are selected for replacement according to the following criteria (and in this order):
     * 1. If invalid (alwasy replace these)
     * 2. If owned, try to keep
     * 3. If shared, try to keep
     * 4. If timestamp is the oldest (smallest), then evict
     */
    uint64_t findBestCandidate(std::vector<ReplacementInfo*> &rInfo) {
        bestCandidate = rInfo[0]->getIndex();
        Rank bestRank = {array[rInfo[0]->getIndex()],
            static_cast<CoherenceReplacementInfo*>(rInfo[0])->getShared(),
            static_cast<CoherenceReplacementInfo*>(rInfo[0])->getOwned(),
            rInfo[0]->getState() };
        if (rInfo[0]->getState() == I)
            return bestCandidate;

        for (int i = 1; i < rInfo.size(); i++) {
            if (rInfo[i]->getState() == I) {
                bestCandidate = rInfo[i]->getIndex();
                return bestCandidate;
            }
            Rank candRank = {array[rInfo[i]->getIndex()],
                static_cast<CoherenceReplacementInfo*>(rInfo[i])->getShared(),
                static_cast<CoherenceReplacementInfo*>(rInfo[i])->getOwned(),
                rInfo[i]->getState() };

            if (candRank.lessThan(bestRank)) {
                bestRank = candRank;
                bestCandidate = rInfo[i]->getIndex();
            }
        }
        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate; }

private:
    uint64_t timestamp;
    uint64_t bestCandidate;
    uint64_t ways;

    std::vector<uint64_t> array;

    struct Rank {
        uint64_t    timestamp;
        bool        shared;
        bool        owned;
        State       state;

        void reset() {
            state       = I;
            shared      = false;
            timestamp   = 0;
            owned       = false;
        }

        inline bool lessThan(const Rank& other) const {
            if (!shared && other.shared) return true;
            else if (shared && !other.shared) return false;
            else if (!owned && other.owned) return true;
            else if (owned && !other.owned) return false;
            else return timestamp < other.timestamp;
        }
    };
};

/* ------------------------------------------------------------------------------------------
 *  LFU
 * ------------------------------------------------------------------------------------------*/
class LFU : public ReplacementPolicy {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(LFU, "memHierarchy", "replacement.lfu", SST_ELI_ELEMENT_VERSION(1,0,0),
            "least-frequently-used replacement policy, recently used accesses are more heavily weighted", SST::MemHierarchy::ReplacementPolicy);


    LFU(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), timestamp(1), bestCandidate(0) {
        ways = associativity;

        array.resize(lines, (LFUInfo){0,0});
    }

    virtual ~LFU() { }

    /* Too expensive to constantly dynamic_cast. Check once during construction instead. */
    bool checkCompatibility(ReplacementInfo * rInfo) { return true; } // No cast

    // timestamp = (total accesses * timestamp + timestamp) / (accesses + 1)
    // timestamp increments by 1000 every time to make sure there's sufficient space between timestamps
    void update(uint64_t id, ReplacementInfo * rInfo) {
        array[id].ts = (array[id].acc*array[id].ts + timestamp)/(array[id].acc + 1);
        array[id].acc++;
        timestamp += 1000;
    }

    uint64_t findBestCandidate(vector<ReplacementInfo*> &rInfo) {
        bestCandidate = rInfo[0]->getIndex();
        LFUInfo bestLFU = array[rInfo[0]->getIndex()];

        if (rInfo[0]->getState() == I) { return bestCandidate; }

        for (int i = 1; i < rInfo.size(); i++) {
            if (rInfo[i]->getState() == I)  {
                bestCandidate = rInfo[i]->getIndex();
                return bestCandidate;
            }
            LFUInfo candLFU = array[rInfo[i]->getIndex()];

            if (candLFU.lessThan(bestLFU, timestamp)) {
                bestLFU = candLFU;
                bestCandidate = rInfo[i]->getIndex();
            }
        }
        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate; }

    //void replaced(uint64_t id) { array[id].acc = 0; }
    void replaced(uint64_t id) { array[id].acc = 0; }
private:

    struct LFUInfo {
        uint64_t ts;    // timestamp, function of accesses with more recent ones being more heavily weighted
        uint64_t acc;   // accesses

        inline bool lessThan(LFUInfo& other, const uint64_t curTs) {
            if (acc == 0) return true;
            if (other.acc == 0) return false;
            uint64_t ownInvFreq = (curTs - ts)/acc; //inverse frequency, lower is better
            uint64_t otherInvFreq = (curTs - other.ts)/other.acc;
            return ownInvFreq > otherInvFreq;
            return false;
        }
    };

    std::vector<LFUInfo> array;
    uint64_t ways;

    uint64_t timestamp;
    uint64_t bestCandidate;

};

class LFUOpt : public ReplacementPolicy {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(LFUOpt, "memHierarchy", "replacement.lfu-opt", SST_ELI_ELEMENT_VERSION(1,0,0),
            "least-frequently-used replacement policy, recently used accesses are more heavily weighted. Also considers coherence state in replacement decision", SST::MemHierarchy::ReplacementPolicy);


    LFUOpt(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), timestamp(1), bestCandidate(0) {
        ways = associativity;

        array.resize(lines, (LFUInfo){0,0});
    }

    virtual ~LFUOpt() { }

    /* Too expensive to constantly dynamic_cast. Check once during construction instead. */
    bool checkCompatibility(ReplacementInfo * rInfo) {
        if (dynamic_cast<CoherenceReplacementInfo*>(rInfo))
            return true;
        return false;
    }

    // timestamp = (total accesses * timestamp + timestamp) / (accesses + 1)
    // timestamp increments by 1000 every time to make sure there's sufficient space between timestamps
    void update(uint64_t id, ReplacementInfo * rInfo) {
        array[id].ts = (array[id].acc*array[id].ts + timestamp)/(array[id].acc + 1);
        array[id].acc++;
        timestamp += 1000;
    }

    uint64_t findBestCandidate(vector<ReplacementInfo*> &rInfo) {
        bestCandidate = rInfo[0]->getIndex();
        Rank bestRank = {array[rInfo[0]->getIndex()],
            static_cast<CoherenceReplacementInfo*>(rInfo[0])->getShared(),
            static_cast<CoherenceReplacementInfo*>(rInfo[0])->getOwned(),
            rInfo[0]->getState() };
        if (rInfo[0]->getState() == I)
            return bestCandidate;

        for (int i = 1; i < rInfo.size(); i++) {
            if (rInfo[i]->getState() == I) {
                bestCandidate = rInfo[i]->getIndex();
                return bestCandidate;
            }
            Rank candRank = {array[rInfo[i]->getIndex()],
                static_cast<CoherenceReplacementInfo*>(rInfo[i])->getShared(),
                static_cast<CoherenceReplacementInfo*>(rInfo[i])->getOwned(),
                rInfo[i]->getState() };
            if (candRank.lessThan(bestRank, timestamp)) {
                bestRank = candRank;
                bestCandidate = rInfo[i]->getIndex();
            }
        }
        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate; }

    //void replaced(uint64_t id) { array[id].acc = 0; }
    void replaced(uint64_t id) { array[id].acc = 0; }
private:

    struct LFUInfo {
        uint64_t ts;    // timestamp, function of accesses with more recent ones being more heavily weighted
        uint64_t acc;   // accesses
    };

    std::vector<LFUInfo> array;
    uint64_t ways;

    uint64_t timestamp;
    uint64_t bestCandidate;

    struct Rank {
        LFUInfo lfuInfo;
        bool shared;
        bool owned;
        State state;

        void reset() {
            state = I;
            shared = false;
            owned = false;
            lfuInfo = (LFUInfo){0, 0};
        }

        inline bool lessThan(const Rank& other, const uint64_t curTs) const {
            if(state == I) return true;
            if (!shared && other.shared) {
                return true;
            } else if (shared && !other.shared) {
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
            return false;
        }
    };
};



/* ------------------------------------------------------------------------------------------
 *  MRU
 * ------------------------------------------------------------------------------------------*/
class MRU : public ReplacementPolicy {
private:
    uint64_t                timestamp;
    int32_t                 bestCandidate;
    std::vector<uint64_t>   array;
    uint64_t                ways;

    struct Rank {
        uint64_t  timestamp;
        State     state;

        void reset() {
            state       = I;
            timestamp   = 0;
        }

        inline bool biggerThan(const Rank& other) const {
            if (state == I)
                return true;
            else
                return timestamp > other.timestamp;
        }
    };

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MRU, "memHierarchy", "replacement.mru", SST_ELI_ELEMENT_VERSION(1,0,0),
            "most-recently-used replacement policy", SST::MemHierarchy::ReplacementPolicy);


    MRU(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), timestamp(1), bestCandidate(0) {
        ways = associativity;

        array.resize(lines, 0);
    }

    virtual ~MRU() { }

    /* Too expensive to constantly dynamic_cast. Check once during construction instead. */
    bool checkCompatibility(ReplacementInfo * rInfo) { return true; } // No cast

    void update(uint64_t id, ReplacementInfo * rInfo) { array[id] = timestamp++; }

    //void replaced(uint64_t id) { array[id] = 0; }
    void replaced(uint64_t id) { array[id] = 0; }

    uint64_t findBestCandidate(vector<ReplacementInfo*> &rInfo) {
        bestCandidate = rInfo[0]->getIndex();
        Rank bestRank = {array[rInfo[0]->getIndex()], rInfo[0]->getState() };
        if (rInfo[0]->getState() == I)
            return bestCandidate;

        for (int i = 1; i < rInfo.size(); i++) {
            if (rInfo[i]->getState() == I) {
                bestCandidate = rInfo[i]->getIndex();
                return bestCandidate;
            }
            Rank candRank = {array[rInfo[i]->getIndex()], rInfo[i]->getState() };
            if (candRank.biggerThan(bestRank)) {
                bestRank = candRank;
                bestCandidate = rInfo[i]->getIndex();
            }
        }
        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate;}

};


class MRUOpt : public ReplacementPolicy {
private:
    uint64_t                timestamp;
    int32_t                 bestCandidate;
    std::vector<uint64_t>   array;
    uint64_t                ways;

    struct Rank {
        uint64_t  timestamp;
        bool      shared;
        bool      owned;
        State     state;

        void reset() {
            state       = I;
            shared      = false;
            owned       = false;
            timestamp   = 0;
        }

        inline bool biggerThan(const Rank& other) const {
            if(state == I) return true;
            else{
                if (!shared && other.shared) return true;
                else if (shared && !other.shared) return false;
                else if (owned && !other.owned) return false;
                else if (!owned && other.owned) return true;
                else return timestamp > other.timestamp;
            }
        }
    };

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MRUOpt, "memHierarchy", "replacement.mru-opt", SST_ELI_ELEMENT_VERSION(1,0,0),
            "most-recently-used replacement policy, with consideration for coherence state", SST::MemHierarchy::ReplacementPolicy);


    MRUOpt(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), timestamp(1), bestCandidate(0) {
        ways = associativity;

        array.resize(lines, 0);
    }

    virtual ~MRUOpt() { }

    /* Too expensive to constantly dynamic_cast. Check once during construction instead. */
    bool checkCompatibility(ReplacementInfo * rInfo) {
        if (dynamic_cast<CoherenceReplacementInfo*>(rInfo))
            return true;
        return false;
    }

    void update(uint64_t id, ReplacementInfo * rInfo) { array[id] = timestamp++; }

    //void replaced(uint64_t id) { array[id] = 0; }
    void replaced(uint64_t id) { array[id] = 0; }

    uint64_t findBestCandidate(vector<ReplacementInfo*> &rInfo) {
        bestCandidate = rInfo[0]->getIndex();
        Rank bestRank = {array[rInfo[0]->getIndex()],
            static_cast<CoherenceReplacementInfo*>(rInfo[0])->getShared(),
            static_cast<CoherenceReplacementInfo*>(rInfo[0])->getOwned(),
            rInfo[0]->getState() };
        if (rInfo[0]->getState() == I)
            return bestCandidate;

        for (int i = 1; i < rInfo.size(); i++) {
            if (rInfo[i]->getState() == I) {
                bestCandidate = rInfo[i]->getIndex();
                return bestCandidate;
            }
            Rank candRank = {array[rInfo[i]->getIndex()],
                static_cast<CoherenceReplacementInfo*>(rInfo[i])->getShared(),
                static_cast<CoherenceReplacementInfo*>(rInfo[i])->getOwned(),
                rInfo[i]->getState() };
            if (candRank.biggerThan(bestRank)) {
                bestRank = candRank;
                bestCandidate = rInfo[i]->getIndex();
            }
        }
        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate; }

};



/* ------------------------------------------------------------------------------------------
 *  Random
 * ------------------------------------------------------------------------------------------*/
class Random : public ReplacementPolicy {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(Random, "memHierarchy", "replacement.random", SST_ELI_ELEMENT_VERSION(1,0,0),
            "random replacement policy", SST::MemHierarchy::ReplacementPolicy);

    SST_ELI_DOCUMENT_PARAMS(
            {"seed_a",  "Seed for random number generator", "1"},
            {"seed_b",  "Seed for random number generator", "1"} )


    Random(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), bestCandidate(0) {
        ways = associativity;
        uint64_t seeda = params.find<uint64_t>("seed_a", 1);
        uint64_t seedb = params.find<uint64_t>("seed_b", 1);
        gen = new SST::RNG::MarsagliaRNG(seeda, seedb);
    }

    virtual ~Random() {
        delete gen;
    }

    /* Too expensive to constantly dynamic_cast. Check once during construction instead. */
    bool checkCompatibility(ReplacementInfo * rInfo) {
        return true; // No cast
    }


    void update(uint64_t id, ReplacementInfo * rInfo){}
    void replaced(uint64_t id){}

    // Return an empty slot if one exists, otherwise return a random candidate
    uint64_t findBestCandidate(std::vector<ReplacementInfo*> &rInfo) {
        // Check for empty line
        for (uint64_t i = 0; i < rInfo.size(); i++) {
            if (rInfo[i]->getState() == I) {
                bestCandidate = rInfo[i]->getIndex();
                return bestCandidate;
            }
        }
        bestCandidate = rInfo[(gen->generateNextUInt64() % ways)]->getIndex();
        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate; }

private:
    uint64_t bestCandidate;
    uint64_t ways;
    SST::RNG::MarsagliaRNG* gen;
};

/* ------------------------------------------------------------------------------------------
 *  Not most recently used (nmru)
 *  - Replacement algorithm assumes indices are contiguous for the set
 * ------------------------------------------------------------------------------------------*/

class NMRU : public ReplacementPolicy {
private:
    uint64_t              bestCandidate;
    std::vector<uint64_t> array;
    uint64_t              ways;
    SST::RNG::MarsagliaRNG* gen;

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(NMRU, "memHierarchy", "replacement.nmru", SST_ELI_ELEMENT_VERSION(1,0,0),
            "not-most-recently-used, random replacement among all but the most recently used line in a set", SST::MemHierarchy::ReplacementPolicy);

    SST_ELI_DOCUMENT_PARAMS(
            {"seed_a",  "Seed for random number generator", "1"},
            {"seed_b",  "Seed for random number generator", "1"} )


    NMRU(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), bestCandidate(0) {
        ways = associativity;
        uint64_t sets = lines/associativity;
        array.resize(sets, 0);

        uint64_t seeda = params.find<uint64_t>("seed_a", 1);
        uint64_t seedb = params.find<uint64_t>("seed_b", 1);
        gen = new SST::RNG::MarsagliaRNG(seeda, seedb);
    }

    virtual ~NMRU() {
        delete gen;
    }

    /* Too expensive to constantly dynamic_cast. Check once during construction instead. */
    bool checkCompatibility(ReplacementInfo * rInfo) {
        return true; // No cast
    }

    void update(uint64_t id, ReplacementInfo * rInfo) { array[id/ways] = id % ways; }
    //void replaced(uint64_t id) {}
    void replaced(uint64_t id) { }

    // Return an empty slot if one exists, otherwise return any slot that is not the most-recently used in the set
    uint64_t findBestCandidate(std::vector<ReplacementInfo*> &rInfo) {
        for (uint64_t i = 0; i < ways; i++) {
            if (rInfo[i]->getState() == I) {
                bestCandidate = rInfo[i]->getIndex();
                return bestCandidate;
            }
        }
        uint64_t setBegin = rInfo[0]->getIndex();
        uint64_t index = gen->generateNextUInt64() % (ways-1);
        if (index < array[setBegin/ways])
            bestCandidate = setBegin + index;
        else
            bestCandidate = setBegin + index + 1;

        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate; }
};


}}



#endif	/* REPLACEMENT_PROTOCOL_H */

