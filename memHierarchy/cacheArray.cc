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

/*
 * File:   cacheArray.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include "cacheArray.h"
#include <vector>

namespace SST { namespace MemHierarchy {

/* Set Associative Array Class */
SetAssociativeArray::SetAssociativeArray(Output* dbg, unsigned int numLines, unsigned int lineSize, unsigned int associativity, ReplacementMgr* rm, HashFunction* hf, bool sharersAware) :
    CacheArray(dbg, numLines, associativity, lineSize, rm, hf, sharersAware, true) 
    { 
        setStates = new State[associativity];
        setSharers = new unsigned int[associativity];
        setOwned = new bool[associativity];
    }


SetAssociativeArray::~SetAssociativeArray() {
    delete [] setStates;
    delete [] setSharers;
    delete [] setOwned;
}

int SetAssociativeArray::find(const Addr baseAddr, bool update) {
    Addr lineAddr = toLineAddr(baseAddr);
    int set = hash_->hash(0, lineAddr) & setMask_;
    int setBegin = set * associativity_;
    int setEnd = setBegin + associativity_;
   
    for (int i = setBegin; i < setEnd; i++) {
        if (lines_[i]->getBaseAddr() == baseAddr) {
            if (update) replacementMgr_->update(i);
            return i;
        }
    }
    return -1;
}

CacheArray::CacheLine* SetAssociativeArray::findReplacementCandidate(const Addr baseAddr, bool cache) {
    int index = preReplace(baseAddr);
    return lines_[index];
}

unsigned int SetAssociativeArray::preReplace(const Addr baseAddr) {
    Addr lineAddr   = toLineAddr(baseAddr);
    int set         = hash_->hash(0, lineAddr) & setMask_;
    int setBegin    = set * associativity_;
    
    for (unsigned int id = 0; id < associativity_; id++) {
        setStates[id] = lines_[id+setBegin]->getState();
        setSharers[id] = lines_[id+setBegin]->numSharers();
        setOwned[id] = lines_[id+setBegin]->ownerExists();
    }
    return replacementMgr_->findBestCandidate(setBegin, setStates, setSharers, setOwned, sharersAware_? true: false);
}

void SetAssociativeArray::replace(const Addr baseAddr, unsigned int candidate_id, bool ignoreParam1, unsigned int ignoreParam2) {
    replacementMgr_->replaced(candidate_id);
    lines_[candidate_id]->reset();
    lines_[candidate_id]->setBaseAddr(baseAddr);
    replacementMgr_->update(candidate_id);
}

void SetAssociativeArray::deallocate(unsigned int index) {
    replacementMgr_->replaced(index);
    lines_[index]->reset();
}

/* Dual Set Associative Array Class */
DualSetAssociativeArray::DualSetAssociativeArray(Output* dbg, unsigned int lineSize, HashFunction * hf, bool sharersAware, unsigned int dirNumLines, 
        unsigned int dirAssociativity, ReplacementMgr * dirRp, unsigned int cacheNumLines, unsigned int cacheAssociativity, ReplacementMgr * cacheRp) :
        CacheArray(dbg, dirNumLines, dirAssociativity, lineSize, dirRp, hf, sharersAware, false)
    {
        // Set up data cache
        cacheNumLines_  = cacheNumLines;
        cacheAssociativity_ = cacheAssociativity;
        cacheReplacementMgr_ = cacheRp;
        cacheNumSets_   = cacheNumLines_ / cacheAssociativity_;
        cacheSetMask_   = cacheNumSets_ - 1;
        dataLines_.resize(cacheNumLines_);
        for (unsigned int i = 0; i < cacheNumLines_; i++) {
            dataLines_[i] = new DataLine(lineSize_, i, dbg_);
        }

        dirSetStates    = new State[dirAssociativity];
        dirSetSharers   = new unsigned int[dirAssociativity];
        dirSetOwned     = new bool[dirAssociativity];
        cacheSetStates  = new State[cacheAssociativity];
        cacheSetSharers = new unsigned int[cacheAssociativity];
        cacheSetOwned   = new bool[cacheAssociativity];
    }


int DualSetAssociativeArray::find(const Addr baseAddr, bool update) {
    Addr lineAddr = toLineAddr(baseAddr);
    int set = hash_->hash(0, lineAddr) & setMask_;
    int setBegin = set * associativity_;
    int setEnd = setBegin + associativity_;
   
    for (int i = setBegin; i < setEnd; i++) {
        if (lines_[i]->getBaseAddr() == baseAddr) {
            if (update) {
                replacementMgr_->update(i);
                if (lines_[i]->getDataLine() != NULL) {
                    cacheReplacementMgr_->update(lines_[i]->getDataLine()->getIndex());
                }
            }
            return i;
        }
    }
    return -1;
}

CacheArray::CacheLine * DualSetAssociativeArray::findReplacementCandidate(const Addr baseAddr, bool cache) {
    int index = cache ? preReplaceDir(baseAddr) : preReplaceCache(baseAddr);

    // Handle invalid case for cache allocations -> then dataLines_[index]->getDirIndex() == -1
    // and we can just link that cache entry to the dir entry for baseAddr
    if (!cache && dataLines_[index]->getDirIndex() == -1) {
        int dirIndex = find(baseAddr, false);
        CacheLine * currentDirEntry = lines_[dirIndex];
        if (currentDirEntry->getDataLine() != NULL) {
            dbg_->fatal(CALL_INFO, -1, "Error: Attempting to allocate a data line for a directory entry that already has an associated data line. Addr = 0x%" PRIx64 "\n", baseAddr);
        }
        currentDirEntry->setDataLine(dataLines_[index]);    // Don't need to set dirIndex, "replace" will do that
        return currentDirEntry;
    }
    return cache ? lines_[index] : lines_[dataLines_[index]->getDirIndex()];
}

unsigned int DualSetAssociativeArray::preReplaceDir(const Addr baseAddr) {
    Addr lineAddr   = toLineAddr(baseAddr);
    int set         = hash_->hash(0, lineAddr) & setMask_;
    int setBegin    = set * associativity_;
    
    for (unsigned int id = 0; id < associativity_; id++) {
        dirSetStates[id]    = lines_[id+setBegin]->getState();
        dirSetSharers[id]   = lines_[id+setBegin]->numSharers();
        dirSetOwned[id]     = lines_[id+setBegin]->ownerExists();
    }
    return replacementMgr_->findBestCandidate(setBegin, dirSetStates, dirSetSharers, dirSetOwned, sharersAware_);
}

void DualSetAssociativeArray::replace(const Addr baseAddr, unsigned int id, bool cache, unsigned int newLinkID) {
    if (cache) {
        replacementMgr_->replaced(id);
        if (lines_[id]->getDataLine() != NULL) {
            deallocateCache(lines_[id]->getDataLine()->getIndex());
        }
        lines_[id]->reset();
        lines_[id]->setBaseAddr(baseAddr);
        replacementMgr_->update(id); 
    } else {
        cacheReplacementMgr_->replaced(id);
        if (dataLines_[id]->getDirIndex() != -1) {
            DataLine * dataLine = NULL;
            lines_[dataLines_[id]->getDirIndex()]->setDataLine(dataLine);
        }
        dataLines_[id]->setDirIndex(newLinkID);
        lines_[newLinkID]->setDataLine(dataLines_[id]);
        cacheReplacementMgr_->update(id);
    }
}

unsigned int DualSetAssociativeArray::preReplaceCache(const Addr baseAddr) {
    Addr lineAddr   = toLineAddr(baseAddr);
    int set         = hash_->hash(0, lineAddr) & cacheSetMask_;
    int setBegin    = set * cacheAssociativity_;
    
    for (unsigned int id = 0; id < cacheAssociativity_; id++) {
        int dirIndex = dataLines_[id+setBegin]->getDirIndex();
        if (dirIndex == -1) {
            cacheSetStates[id] = I;
            cacheSetSharers[id] = 0;
            cacheSetOwned[id] = false;
        } else {
            cacheSetStates[id]  = lines_[dirIndex]->getState();
            cacheSetSharers[id] = lines_[dirIndex]->numSharers();
            cacheSetOwned[id]   = lines_[dirIndex]->ownerExists();
        }
    }
    return cacheReplacementMgr_->findBestCandidate(setBegin, cacheSetStates, cacheSetSharers, cacheSetOwned, sharersAware_);
}

void DualSetAssociativeArray::deallocateCache(unsigned int index) {
    cacheReplacementMgr_->replaced(index);
    dataLines_[index]->setDirIndex(-1);
}


/* Cache Array Class */
void CacheArray::printConfiguration() {
    dbg_->debug(_INFO_, "Sets: %d \n", numSets_);
    dbg_->debug(_INFO_, "Lines: %d \n", numLines_);
    dbg_->debug(_INFO_, "Line size: %d \n", lineSize_);
    dbg_->debug(_INFO_, "Set mask: %d \n", setMask_);
    dbg_->debug(_INFO_, "Associativity: %i \n\n", associativity_);
}

void CacheArray::errorChecking() {
    if(0 == numLines_ || 0 == numSets_)     dbg_->fatal(CALL_INFO, -1, "Cache size and/or number of sets not greater than zero. Number of lines = %d, Number of sets = %d.\n", numLines_, numSets_);
    // TODO relax this, use mod instead of setmask_
    if(!isPowerOfTwo(numSets_))             dbg_->fatal(CALL_INFO, -1, "Number of sets is not a power of two. Number of sets = %d.\n", numSets_);
    if((numSets_ * associativity_) != numLines_) dbg_->fatal(CALL_INFO, -1, "Wrong configuration.  Make sure numSets * associativity = Size/cacheLineSize. Number of sets = %d, Associtaivity = %d, Number of lines = %d.\n", numSets_, associativity_, numLines_);
    if(associativity_ < 1)                  dbg_->fatal(CALL_INFO, -1, "Associativity has to be greater than zero. Associativity = %d\n", associativity_);
}

}}

