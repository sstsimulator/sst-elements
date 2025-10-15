// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

#include <cstddef>
#include <cstdint>
#include <map>
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
        Output*         debug_;
        unsigned int    num_sets_;
        unsigned int    num_lines_;
        unsigned int    associativity_;
        unsigned int    line_offset_;
        uint32_t        line_size_;
        ReplacementPolicy* replacement_mgr_;
        HashFunction*   hash_;
        Addr            slice_size_; // For cache slices
        Addr            slice_step_; // For cache slices
        unsigned int    banks_;
        vector<T*>      lines_; // The actual cache
        State* setStates;
        std::map<unsigned int, std::vector<ReplacementInfo*> > rInfo;   // Lookup a vector of replacementInfo by set ID
    public:

        CacheArray(Output* dbg, unsigned int numLines, unsigned int associativity, uint32_t lineSize, ReplacementPolicy* replacementMgr, HashFunction* hash);

        /** Destructor - Delete all cache line objects */
        ~CacheArray();

    /**** Address/bank/etc. computations */

        /** Get line size.  Should not change at runtime */
        uint32_t getLineSize() { return line_size_; }

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

    /**** Cache iterators */
        struct cache_itr {
            public:
                cache_itr(const CacheArray<T>* c, unsigned idx) : cache_(c), idx_(idx) {}
                bool operator!=(const cache_itr& rhs) const { return idx_ != rhs.idx_; }
                bool operator==(const cache_itr& rhs) const { return idx_ == rhs.idx_; }
                const cache_itr& operator++() { ++idx_; return *this; }
                const cache_itr& operator--() { --idx_; return *this; }
                const T* operator*() const { return cache_->lines_[idx_]; }
                T* operator*() { return cache_->lines_[idx_]; }
            private:
                const CacheArray<T>* cache_;
                unsigned idx_;
        };
        cache_itr begin() const { return cache_itr(this, 0); }
        cache_itr end() const { return cache_itr(this, num_lines_); }

    /* Serialization support */
    CacheArray() = default;
    void serialize_order(SST::Core::Serialization::serializer& ser);
};

/************* Function definitions *****************/

template <class T>
CacheArray<T>::CacheArray(Output* dbg, unsigned int numLines, unsigned int associativity, uint32_t lineSize, ReplacementPolicy* replacementMgr, HashFunction* hash) :
    debug_(dbg), num_lines_(numLines), associativity_(associativity), line_size_(lineSize), replacement_mgr_(replacementMgr), hash_(hash) {

    // Error check parameters
    if (num_lines_ == 0)
        debug_->fatal(CALL_INFO, -1, "CacheArray, Error: number of lines is 0. Must be greater than 0.\n");

    if (associativity_ == 0)
        debug_->fatal(CALL_INFO, -1, "CacheArray, Error: associativity is 0. Use 1 for direct mapped, 2 or more for set-associative.\n");

    num_sets_ = num_lines_ / associativity_;

    if (num_sets_ == 0)
        debug_->fatal(CALL_INFO, -1, "CacheArray, Error: number of sets (number of lines / associativity) is 0. Must be at least 1. Number of lines = %u. Associativity = %u.\n",
                        num_lines_, associativity_);
    if ((num_sets_ * associativity_) != num_lines_)
        debug_->fatal(CALL_INFO, -1, "CacheArray, Error: The number of cachelines is not divisible by the cache associativity. Ensure (lines mod associativity = 0). Number of lines = %u. Associativity = %u\n",
                num_lines_, associativity_);

    line_offset_ = log2Of(line_size_);
    lines_.resize(num_lines_);

    // Set later using setter functions
    slice_step_ = 1;
    slice_size_ = 1;
    banks_ = 1;

    for (unsigned int i = 0; i < num_lines_; i++) {
        lines_[i] = new T(line_size_, i);
    }

    // Construct rInfo
    for (unsigned int i = 0; i < num_sets_; i++) {
        std::vector<ReplacementInfo*> setInfo;
        for (unsigned int j = 0; j < associativity; j++)
            setInfo.push_back(lines_[i*associativity + j]->getReplacementInfo());
        rInfo.insert(std::make_pair(i, setInfo));
    }
    ReplacementInfo * info = rInfo.find(0)->second.front();
    if (!replacement_mgr_->checkCompatibility(info))
        debug_->fatal(CALL_INFO, -1, "CacheArray, Error: The replacement policy expects cache line state that is not provided by the cache line type of this cache. Check the type of the ReplacementInfo returned by the coherence protocol's line type and the ReplacementInfo type expected by the replacement policy.\n");

    setStates = new State[associativity_];
}

template <class T>
CacheArray<T>::~CacheArray() {
    for (size_t i = 0; i < lines_.size(); i++)
        delete lines_[i];
    delete [] setStates;
}

template <class T>
Addr CacheArray<T>::toLineAddr(Addr addr) {
    Addr shift = addr >> line_offset_;
    Addr step = shift / slice_step_;
    Addr offset = shift % slice_size_;
    return step * slice_size_ + offset;
}

template <class T>
T* CacheArray<T>::lookup(const Addr addr, bool updateReplacement) {
    Addr laddr = toLineAddr(addr);
    int set = hash_->hash(0, laddr) % num_sets_;
    int setBegin = set * associativity_;
    int setEnd = setBegin + associativity_;

    for (int i = setBegin; i < setEnd; i++) {
        if (lines_[i]->getAddr() == addr) {
            if (updateReplacement)
                replacement_mgr_->update(i, lines_[i]->getReplacementInfo());
            return lines_[i];
        }
    }
    return nullptr; // Not found
}

template <class T>
T * CacheArray<T>::findReplacementCandidate(Addr addr) {
    Addr laddr = toLineAddr(addr);
    int set = hash_->hash(0, laddr) % num_sets_;

    unsigned int id = replacement_mgr_->findBestCandidate(rInfo[set]);

    return lines_[id];
}

template <class T>
void CacheArray<T>::replace(Addr addr, T* candidate) {
    unsigned int index = candidate->getIndex();
    replacement_mgr_->replaced(index);
    candidate->reset();
    candidate->setAddr(addr);
    replacement_mgr_->update(index, lines_[index]->getReplacementInfo());
}

template <class T>
void CacheArray<T>::deallocate(T* candidate) {
    unsigned int index = candidate->getIndex();
    replacement_mgr_->replaced(index);
    candidate->reset();
}

template <class T>
void CacheArray<T>::setSliceAware(Addr size, Addr step) {
    slice_size_ = size >> line_offset_;
    slice_step_ = step >> line_offset_;
    if (slice_size_ == 0) slice_size_ = 1;
    if (slice_step_ == 0) slice_step_ = 1;
}

template <class T>
void CacheArray<T>::setBanked(unsigned int numBanks) {
    banks_ = numBanks;
}

template <class T>
void CacheArray<T>::printCacheArray(Output &out) {
    for (unsigned int i = 0; i < num_lines_; i++) {
        out.output("   %u %s\n", i, lines_[i]->getString().c_str());
    }
}

template <class T>
void CacheArray<T>::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(debug_); // TODO - better to reinit after restart?
    SST_SER(num_sets_);
    SST_SER(num_lines_);
    SST_SER(associativity_);
    SST_SER(line_offset_);
    SST_SER(line_size_);
    SST_SER(replacement_mgr_);
    SST_SER(hash_);
    SST_SER(slice_size_);
    SST_SER(slice_step_);
    SST_SER(banks_);
    SST_SER(lines_);
    SST_SER(setStates);
    SST_SER(rInfo);
}

}}
#endif	/* CACHEARRAY_H */
