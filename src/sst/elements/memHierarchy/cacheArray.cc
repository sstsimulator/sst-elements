// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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

CacheArray::CacheLine* SetAssociativeArray::lookup(const Addr baseAddr, bool update) {
    Addr lineAddr = toLineAddr(baseAddr);
    int set = hash_->hash(0, lineAddr) % numSets_;
    int setBegin = set * associativity_;
    int setEnd = setBegin + associativity_;
   
    for (int i = setBegin; i < setEnd; i++) {
        if (lines_[i]->getBaseAddr() == baseAddr) {
            if (update) replacementMgr_->update(i);
            return lines_[i];
        }
    }
    return nullptr;
}

CacheArray::CacheLine* SetAssociativeArray::findReplacementCandidate(const Addr baseAddr, bool cache) {
    int index = preReplace(baseAddr);
    return lines_[index];
}

unsigned int SetAssociativeArray::preReplace(const Addr baseAddr) {
    Addr lineAddr   = toLineAddr(baseAddr);
    int set         = hash_->hash(0, lineAddr) % numSets_;
    int setBegin    = set * associativity_;
    
    for (unsigned int id = 0; id < associativity_; id++) {
        setStates[id] = lines_[id+setBegin]->getState();
        setSharers[id] = lines_[id+setBegin]->numSharers();
        setOwned[id] = lines_[id+setBegin]->ownerExists();
    }
    return replacementMgr_->findBestCandidate(setBegin, setStates, setSharers, setOwned, sharersAware_? true: false);
}

void SetAssociativeArray::replace(const Addr baseAddr, CacheArray::CacheLine * candidate, CacheArray::DataLine * dataCandidate) {
    unsigned int index = candidate->getIndex();
    replacementMgr_->replaced(index);
    candidate->reset();
    candidate->setBaseAddr(baseAddr);
    replacementMgr_->update(index);
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


CacheArray::CacheLine* DualSetAssociativeArray::lookup(const Addr baseAddr, bool update) {
    Addr lineAddr = toLineAddr(baseAddr);
    int set = hash_->hash(0, lineAddr) % numSets_;
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
            return lines_[i];
        }
    }
    return nullptr;
}

CacheArray::CacheLine * DualSetAssociativeArray::findReplacementCandidate(const Addr baseAddr, bool cache) {
    int index = cache ? preReplaceDir(baseAddr) : preReplaceCache(baseAddr);

    // Handle invalid case for cache allocations -> then dataLines_[index]->getDirLine() == nullptr
    // and we can just link that cache entry to the dir entry for baseAddr
    if (!cache && dataLines_[index]->getDirLine() == nullptr) {
        CacheLine * currentDirEntry = lookup(baseAddr, false);
        if (currentDirEntry->getDataLine() != NULL) {
            dbg_->fatal(CALL_INFO, -1, "Error: Attempting to allocate a data line for a directory entry that already has an associated data line. Addr = 0x%" PRIx64 "\n", baseAddr);
        }
        currentDirEntry->setDataLine(dataLines_[index]);    // Don't need to set dirLine, "replace" will do that
        return currentDirEntry;
    }
    return cache ? lines_[index] : dataLines_[index]->getDirLine();
}

unsigned int DualSetAssociativeArray::preReplaceDir(const Addr baseAddr) {
    Addr lineAddr   = toLineAddr(baseAddr);
    int set         = hash_->hash(0, lineAddr) % numSets_;
    int setBegin    = set * associativity_;
    
    for (unsigned int id = 0; id < associativity_; id++) {
        dirSetStates[id]    = lines_[id+setBegin]->getState();
        dirSetSharers[id]   = lines_[id+setBegin]->numSharers();
        dirSetOwned[id]     = lines_[id+setBegin]->ownerExists();
    }
    return replacementMgr_->findBestCandidate(setBegin, dirSetStates, dirSetSharers, dirSetOwned, sharersAware_);
}

void DualSetAssociativeArray::replace(const Addr baseAddr, CacheArray::CacheLine * candidate, CacheArray::DataLine * dataCandidate) {
    unsigned int index = candidate->getIndex();
    if (dataCandidate == nullptr) {
        replacementMgr_->replaced(index);
        if (lines_[index]->getDataLine() != NULL) {
            deallocateCache(candidate->getDataLine()->getIndex());
        }
        candidate->reset();
        candidate->setBaseAddr(baseAddr);
        replacementMgr_->update(index); 
    } else {
        unsigned int dIndex = dataCandidate->getIndex();
        cacheReplacementMgr_->replaced(dIndex);
        
        if (dataCandidate->getDirLine() != nullptr) {   // break existing dir->data link
            dataCandidate->getDirLine()->setDataLine(nullptr);
        }
        dataCandidate->setDirLine(candidate); // set new dir->data link
        candidate->setDataLine(dataCandidate);
        cacheReplacementMgr_->update(dIndex);
    }
}

unsigned int DualSetAssociativeArray::preReplaceCache(const Addr baseAddr) {
    Addr lineAddr   = toLineAddr(baseAddr);
    int set         = hash_->hash(0, lineAddr) % cacheNumSets_;
    int setBegin    = set * cacheAssociativity_;
    
    for (unsigned int id = 0; id < cacheAssociativity_; id++) {
        int dirIndex = dataLines_[id+setBegin]->getDirLine() ? dataLines_[id+setBegin]->getDirLine()->getIndex() : -1;
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
    dataLines_[index]->setDirLine(nullptr);
}


/* Cache Array Class */
void CacheArray::printConfiguration() {
    dbg_->debug(_INFO_, "Sets: %d \n", numSets_);
    dbg_->debug(_INFO_, "Lines: %d \n", numLines_);
    dbg_->debug(_INFO_, "Line size: %d \n", lineSize_);
    dbg_->debug(_INFO_, "Associativity: %i \n\n", associativity_);
}

void CacheArray::errorChecking() {
    if(0 == numLines_ || 0 == numSets_)     dbg_->fatal(CALL_INFO, -1, "Cache size and/or number of sets not greater than zero. Number of lines = %d, Number of sets = %d.\n", numLines_, numSets_);
    // TODO relax this, use mod instead of setmask_
    if((numSets_ * associativity_) != numLines_) dbg_->fatal(CALL_INFO, -1, "Wrong configuration.  Make sure numSets * associativity = Size/cacheLineSize. Number of sets = %d, Associtaivity = %d, Number of lines = %d.\n", numSets_, associativity_, numLines_);
    if(associativity_ < 1)                  dbg_->fatal(CALL_INFO, -1, "Associativity has to be greater than zero. Associativity = %d\n", associativity_);
}

}}

