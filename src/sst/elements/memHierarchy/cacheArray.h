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

#ifndef CACHEARRAY_H
#define CACHEARRAY_H

#include <vector>

#include <sst/core/output.h>

#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/hash.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/replacementManager.h"
#include "sst/elements/memHierarchy/lineTypes.h"

using namespace std;

namespace SST { namespace MemHierarchy {

/*
 * CacheArrays should  be templated on a line type
 * See the comment in lineTypes.h for the required API
 */

template <class T>
class CacheArray {
    protected:
        Output*         dbg_;
        unsigned int    numSets_;
        unsigned int    numLines_;
        unsigned int    associativity_;
        unsigned int    lineOffset_;
        uint32_t        lineSize_;
        ReplacementPolicy* replacementMgr_;
        HashFunction*   hash_;
        Addr            sliceSize_; // For cache slices
        Addr            sliceStep_; // For cache slices
        unsigned int    banks_;
        vector<T*>      lines_; // The actual cache
        State* setStates;
        std::map<unsigned int, std::vector<ReplacementInfo*> > rInfo;   // Lookup a vector of replacementInfo by set ID
    public:

        CacheArray(Output* dbg, unsigned int numLines, unsigned int associativity, uint32_t lineSize, ReplacementPolicy* replacementMgr, HashFunction* hash);

        /** Destructor - Delete all cache line objects */
        virtual ~CacheArray();

    /**** Address/bank/etc. computations */

        /** Get line size.  Should not change at runtime */
        uint32_t getLineSize() { return lineSize_; }

        /** Drop block offset bits (ie. log2(lineSize) */
        Addr toLineAddr(Addr addr);

        /** Return bank num */
        Addr getBank(Addr addr) { return (toLineAddr(addr) % banks_); }

    /**** Cache queries & maintenance */

        /** Function returns the cacheline if found, otherwise a null pointer.
            If updateReplacement is set, the replacement stats are updated */
        T * lookup(Addr addr, bool updateReplacement);

        /** Identify a replacement candidate using the replacement manager */
        T * findReplacementCandidate(Addr addr);

        /** Replace a line with address 'addr' and update its replacement info */
        void replace(Addr addr, T* candidate);

        /** Deallocate a line and notify replacement manager that it's been deallocated */
        void deallocate(T* candidate);

    /**** Configuration and output */
        void setSliceAware(Addr size, Addr step);
        void setBanked(unsigned int numBanks);
        void printCacheArray(Output &out);
};

/************* Function definitions *****************/

template <class T>
CacheArray<T>::CacheArray(Output* dbg, unsigned int numLines, unsigned int associativity, uint32_t lineSize, ReplacementPolicy* replacementMgr, HashFunction* hash) :
    dbg_(dbg), numLines_(numLines), associativity_(associativity), lineSize_(lineSize), replacementMgr_(replacementMgr), hash_(hash) {

    // Error check parameters
    if (numLines_ == 0)
        dbg_->fatal(CALL_INFO, -1, "CacheArray, Error: number of lines is 0. Must be greater than 0.\n");

    if (associativity_ == 0)
        dbg_->fatal(CALL_INFO, -1, "CacheArray, Error: associativity is 0. Use 1 for direct mapped, 2 or more for set-associative.\n");

    numSets_ = numLines_ / associativity_;

    if (numSets_ == 0)
        dbg_->fatal(CALL_INFO, -1, "CacheArray, Error: number of sets (number of lines / associativity) is 0. Must be at least 1. Number of lines = %u. Associativity = %u.\n",
                        numLines_, associativity_);
    if ((numSets_ * associativity_) != numLines_)
        dbg_->fatal(CALL_INFO, -1, "CacheArray, Error: The number of cachelines is not divisible by the cache associativity. Ensure (lines mod associativiy = 0). Number of lines = %u. Associativity = %u\n",
                numLines_, associativity_);

    lineOffset_ = log2Of(lineSize_);
    lines_.resize(numLines_);

    // Set later using setter functions
    sliceStep_ = 1;
    sliceSize_ = 1;
    banks_ = 1;

    for (unsigned int i = 0; i < numLines_; i++) {
        lines_[i] = new T(lineSize_, i);
    }

    // Construct rInfo
    for (unsigned int i = 0; i < numSets_; i++) {
        std::vector<ReplacementInfo*> setInfo;
        for (unsigned int j = 0; j < associativity; j++)
            setInfo.push_back(lines_[i*associativity + j]->getReplacementInfo());
        rInfo.insert(std::make_pair(i, setInfo));
    }
    ReplacementInfo * info = rInfo.find(0)->second.front();
    if (!replacementMgr_->checkCompatibility(info))
        dbg_->fatal(CALL_INFO, -1, "CacheArray, Error: The replacement policy expects cache line state that is not provided by the cache line type of this cache. Check the type of the ReplacementInfo returned by the coherence protocol's line type and the ReplacementInfo type expected by the replacement policy.\n");

    setStates = new State[associativity_];
}

template <class T>
CacheArray<T>::~CacheArray() {
    for (size_t i = 0; i < lines_.size(); i++)
        delete lines_[i];
    delete replacementMgr_;
    delete hash_;
    delete [] setStates;
}

template <class T>
Addr CacheArray<T>::toLineAddr(Addr addr) {
    Addr shift = addr >> lineOffset_;
    Addr step = shift / sliceStep_;
    Addr offset = shift % sliceSize_;
    return step * sliceSize_ + offset;
}

template <class T>
T* CacheArray<T>::lookup(const Addr addr, bool updateReplacement) {
    Addr laddr = toLineAddr(addr);
    int set = hash_->hash(0, laddr) % numSets_;
    int setBegin = set * associativity_;
    int setEnd = setBegin + associativity_;

    for (int i = setBegin; i < setEnd; i++) {
        if (lines_[i]->getAddr() == addr) {
            if (updateReplacement)
                replacementMgr_->update(i, lines_[i]->getReplacementInfo());
            return lines_[i];
        }
    }
    return nullptr; // Not found
}

template <class T>
T * CacheArray<T>::findReplacementCandidate(Addr addr) {
    Addr laddr = toLineAddr(addr);
    int set = hash_->hash(0, laddr) % numSets_;

    unsigned int id = replacementMgr_->findBestCandidate(rInfo[set]);

    return lines_[id];
}

template <class T>
void CacheArray<T>::replace(Addr addr, T* candidate) {
    unsigned int index = candidate->getIndex();
    replacementMgr_->replaced(index);
    candidate->reset();
    candidate->setAddr(addr);
    replacementMgr_->update(index, lines_[index]->getReplacementInfo());
}

template <class T>
void CacheArray<T>::deallocate(T* candidate) {
    unsigned int index = candidate->getIndex();
    replacementMgr_->replaced(index);
    candidate->reset();
}

template <class T>
void CacheArray<T>::setSliceAware(Addr size, Addr step) {
    sliceSize_ = size >> lineOffset_;
    sliceStep_ = step >> lineOffset_;
    if (sliceSize_ == 0) sliceSize_ = 1;
    if (sliceStep_ == 0) sliceStep_ = 1;
}

template <class T>
void CacheArray<T>::setBanked(unsigned int numBanks) {
    banks_ = numBanks;
}

template <class T>
void CacheArray<T>::printCacheArray(Output &out) {
    for (unsigned int i = 0; i < numLines_; i++) {
        out.output("   %u %s\n", i, lines_[i]->getString().c_str());
    }
}

}}
#endif	/* CACHEARRAY_H */
