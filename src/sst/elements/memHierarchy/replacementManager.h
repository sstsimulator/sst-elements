// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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

class ReplacementPolicy : public SubComponent{
    public:
        SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MemHierarchy::ReplacementPolicy, uint64_t, uint64_t)

        ReplacementPolicy(Component* comp, Params& params) : SubComponent(comp) {
            Output out("", 1, 0, Output::STDOUT);
            out.fatal(CALL_INFO, -1, "%s, Error: ReplacementPolicy subcomponents do not support loading via legacy API\n", getName().c_str());
        }
        ReplacementPolicy(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : SubComponent(id) { }
        virtual ~ReplacementPolicy(){}
        
        // Update state
        virtual void update(uint64_t id) = 0;
        virtual void replaced(uint64_t id) = 0;
        
        // Get replacement candidates
        virtual uint64_t getBestCandidate() = 0;
        virtual uint64_t findBestCandidate(uint64_t setBegin, State * state, unsigned int * sharers, bool * owned, bool sharersAware) = 0;
};

/* ------------------------------------------------------------------------------------------
 *  LRU
 * ------------------------------------------------------------------------------------------*/
class LRU : public ReplacementPolicy {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(LRU, "memHierarchy", "replacement.lru", SST_ELI_ELEMENT_VERSION(1,0,0),
            "least-recently-used replacement policy", SST::MemHierarchy::ReplacementPolicy);
    
    LRU(Component* comp, Params& params) : ReplacementPolicy(comp, params) { }

    LRU(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), timestamp(1), bestCandidate(0) {
        ways = associativity;
        
        array.resize(lines, 0);
    }

    virtual ~LRU() {}
    
    /* Record recently used line */
    void update(uint64_t id) { array[id] = timestamp++; }

    /* Record replaced line */
    void replaced(uint64_t id) { array[id] = 0; }
    
    uint64_t findBestCandidate(uint64_t setBegin, State * state, unsigned int * sharers, bool * owned, bool sharersAware) {
        uint64_t setEnd = setBegin + ways;
        bestCandidate = setBegin;
        Rank bestRank = {array[setBegin], (sharersAware)? sharers[0] : 0, (sharersAware)? owned[0] : false, state[0]};
        if (state[0] == I) 
            return bestCandidate;
        setBegin++;
        int i = 1;
        for (uint64_t id = setBegin; id < setEnd; id++) {
            Rank candRank = {array[id], (sharersAware)? sharers[i] : 0, (sharersAware)? owned[i] : false, state[i]};
            if (candRank.lessThan(bestRank)) {
                bestRank = candRank;
                bestCandidate = id;
                if (state[i] == I) 
                    return bestCandidate;
            }
            i++;
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
        uint64_t        timestamp;
        unsigned int    sharers;
        bool            owned;
        State           state;

        void reset() {
            state       = I;
            sharers     = 0;
            timestamp   = 0;
            owned       = false;
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
};

/* ------------------------------------------------------------------------------------------
 *  LFU
 * ------------------------------------------------------------------------------------------*/
class LFU : public ReplacementPolicy {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(LFU, "memHierarchy", "replacement.lfu", SST_ELI_ELEMENT_VERSION(1,0,0),
            "least-frequently-used replacement policy, recently used accesses are more heavily weighted", SST::MemHierarchy::ReplacementPolicy);
    
    LFU(Component* comp, Params& params) : ReplacementPolicy(comp, params) { }

    LFU(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), timestamp(1), bestCandidate(0) {
        ways = associativity;

        array.resize(lines, (LFUInfo){0,0});
    }

    virtual ~LFU() { }
        
    // timestamp = (total accesses * timestamp + timestamp) / (accesses + 1)
    // timestamp increments by 1000 every time to make sure there's sufficient space between timestamps
    void update(uint64_t id) {
        array[id].ts = (array[id].acc*array[id].ts + timestamp)/(array[id].acc + 1);
        array[id].acc++;
        timestamp += 1000;
    }

    uint64_t findBestCandidate(uint64_t setBegin, State * state, unsigned int * sharers, bool * owned, bool sharersAware) {
        uint64_t setEnd = setBegin + ways;
        bestCandidate = setBegin;
        Rank bestRank = {array[setBegin], (sharersAware)? sharers[0] : 0, (sharersAware)? owned[0] : false, state[0] };
        if (state[0] == I) 
            return bestCandidate; 
        
        setBegin++;
        int i = 1;
        for (uint64_t id = setBegin; id < setEnd; id++) {
            Rank candRank = {array[id], (sharersAware)? sharers[i] : 0, (sharersAware)? owned[i] : false, state[i]};
            if (candRank.lessThan(bestRank, timestamp)) {
                   bestRank = candRank;
                bestCandidate = id;
                if (state[i] == I) 
                    return bestCandidate;
            }
            i++;
        }
        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate; }

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
        unsigned int sharers;
        bool owned;
        State state;

        void reset() {
            state = I;
            sharers = 0;
            owned = false;
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
};



/* ------------------------------------------------------------------------------------------
 *  MRU
 * ------------------------------------------------------------------------------------------*/
class MRU : public ReplacementPolicy {
private:
    uint64_t                timestamp;
    int32_t                 bestCandidate;
    std::vector<uint64_t>   array;
    uint64_t                  ways;

    struct Rank {
        uint64_t  timestamp;
        uint      sharers;
        bool      owned;
        State     state;

        void reset() {
            state       = I;
            sharers     = 0;
            owned       = false;
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
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MRU, "memHierarchy", "replacement.mru", SST_ELI_ELEMENT_VERSION(1,0,0),
            "most-recently-used replacement policy", SST::MemHierarchy::ReplacementPolicy);
    
    MRU(Component* comp, Params& params) : ReplacementPolicy(comp, params) { }

    MRU(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), timestamp(1), bestCandidate(0) {
        ways = associativity;

        array.resize(lines, 0);
    }

    virtual ~MRU() { }

    void update(uint64_t id) { array[id] = timestamp++; }
    
    void replaced(uint64_t id) { array[id] = 0; }

    uint64_t findBestCandidate(uint64_t setBegin, State * state, uint * sharers, bool * owned, bool sharersAware) {
        uint64_t setEnd = setBegin + ways;
        bestCandidate = setBegin;
        Rank bestRank = {array[setBegin], (sharersAware)? sharers[0] : 0, (sharersAware)? owned[0] : false, state[0] };
        if (state[0] == I) 
            return bestCandidate; 
        
        setBegin++;
        int i = 1;
        for (uint64_t id = setBegin; id < setEnd; id++) {
            Rank candRank = {array[id], (sharersAware)? sharers[i]: 0, (sharersAware)? owned[i] : false, state[i]};
            if (candRank.biggerThan(bestRank)) {
                bestRank = candRank;
                bestCandidate = id;
                if (state[i] == I) 
                    return bestCandidate;
            }
            i++;
        }
        return 
            bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate;}

};



/* ------------------------------------------------------------------------------------------
 *  Random
 * ------------------------------------------------------------------------------------------*/
class Random : public ReplacementPolicy {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(Random, "memHierarchy", "replacement.rand", SST_ELI_ELEMENT_VERSION(1,0,0),
            "random replacement policy", SST::MemHierarchy::ReplacementPolicy);
    
    SST_ELI_DOCUMENT_PARAMS( 
            {"seed_a",  "Seed for random number generator", "1"}, 
            {"seed_b",  "Seed for random number generator", "1"} )
    
    Random(Component* comp, Params& params) : ReplacementPolicy(comp, params) { }

    Random(ComponentId_t id, Params& params, uint64_t lines, uint64_t associativity) : ReplacementPolicy(id, params, lines, associativity), bestCandidate(0) {
        ways = associativity;
        uint64_t seeda = params.find<uint64_t>("seed_a", 1);
        uint64_t seedb = params.find<uint64_t>("seed_b", 1);
        gen = new SST::RNG::MarsagliaRNG(seeda, seedb);
    }
    
    virtual ~Random() {
        delete gen;
    }

    void update(uint64_t id){}
    void replaced(uint64_t id){}
       
    // Return an empty slot if one exists, otherwise return a random candidate
    uint64_t findBestCandidate(uint64_t setBegin, State * state, unsigned int * sharers, bool * owned, bool sharersAware) {
        // Check for empty line
        for (uint64_t i = 0; i < ways; i++) {
            if (state[i] == I) {
                bestCandidate = setBegin + i;
                return bestCandidate;
            }
        }
        bestCandidate = (gen->generateNextUInt64() % ways) + setBegin;
        return bestCandidate;
    }

    uint64_t getBestCandidate() {
        return bestCandidate;
    }

private:
    uint64_t bestCandidate;
    uint64_t ways;
    SST::RNG::MarsagliaRNG* gen;
};

/* ------------------------------------------------------------------------------------------
 *  Not most recently used (nmru)
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
    
    NMRU(Component* comp, Params& params) : ReplacementPolicy(comp, params) { }

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

    void update(uint64_t id) {  array[id/ways] = id % ways;  }
    void replaced(uint64_t id) {}

    // Return an empty slot if one exists, otherwise return any slot that is not the most-recently used in the set
    uint64_t findBestCandidate(uint64_t setBegin, State * state, unsigned int * sharers, bool * owned, bool sharersAware) {
        for (uint64_t i = 0; i < ways; i++) {
            if (state[i] == I) {
                bestCandidate = setBegin + i;
                return bestCandidate;
            }
        }
        uint64_t index = gen->generateNextUInt64() % (ways-1);
        if (index < array[setBegin/ways]) 
            bestCandidate = setBegin + index;
        else 
            bestCandidate = setBegin + index + 1;

        return bestCandidate;
    }

    uint64_t getBestCandidate() { return bestCandidate;}
};


}}



#endif	/* REPLACEMENT_PROTOCOL_H */

