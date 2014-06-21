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
 * File:   cacheArray.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include "cacheArray.h"
#include <vector>
#include "replacementManager.h"

namespace SST { namespace MemHierarchy {

SetAssociativeArray::SetAssociativeArray(Output* _dbg, unsigned int _cacheSize, unsigned int _lineSize, unsigned int _associativity,
                     ReplacementMgr* _rm, HashFunction* _hf, bool _sharersAware) :
                     CacheArray(_dbg, _cacheSize, _cacheSize/_lineSize,
                     _associativity, _lineSize, _rm, _hf, _sharersAware) { }


int SetAssociativeArray::find(const Addr baseAddr, bool update){
    Addr lineAddr = toLineAddr(baseAddr);
    int set = hash_->hash(0, lineAddr) & setMask_;
    int setBegin = set * associativity_;
    int setEnd = setBegin + associativity_;
   
    for (int i = setBegin; i < setEnd; i++) {
        if (lines_[i]->getBaseAddr() == baseAddr){
            if (update) replacementMgr_->update(i);
            return i;
        }
    }
    return -1;
}

unsigned int SetAssociativeArray::preReplace(const Addr baseAddr){
    Addr lineAddr   = toLineAddr(baseAddr);
    replacementMgr_->startReplacement();
    int set         = hash_->hash(0, lineAddr) & setMask_;
    int setBegin    = set * associativity_;
    int setEnd      = setBegin + associativity_;

    for (int id = setBegin; id < setEnd; id++)
        replacementMgr_->recordCandidate(id , sharersAware_? true : false, lines_[id]->getState());
    
    unsigned int candidate_id = replacementMgr_->getBestCandidate();
    replacementMgr_->update(candidate_id);
    return candidate_id;
}

void SetAssociativeArray::replace(const Addr baseAddr, unsigned int candidate_id){
    replacementMgr_->replaced(candidate_id);
    lines_[candidate_id]->clear();
    lines_[candidate_id]->setBaseAddr(baseAddr);
    replacementMgr_->update(candidate_id);
}


void CacheArray::pMembers(){
    d_->debug(_INFO_, "Cache size: %d \n", cacheSize_);
    d_->debug(_INFO_, "Sets: %d \n", numSets_);
    d_->debug(_INFO_, "Lines: %d \n", numLines_);
    d_->debug(_INFO_, "Line size: %d \n", lineSize_);
    d_->debug(_INFO_, "Set mask: %d \n", setMask_);
    d_->debug(_INFO_, "Associativity: %i \n\n", associativity_);
}

void CacheArray::errorChecking(){
    if(0 == cacheSize_ || 0 == numSets_)    _abort(CacheArray, "Cache size and/or number of sets not greater than zero.\n");
    if(!isPowerOfTwo(cacheSize_))           _abort(CacheArray, "Cache size is not a power of two. \n");
    if(!isPowerOfTwo(numSets_))             _abort(CacheArray, "Number of sets is not a power of two \n");
    if((numSets_ * associativity_) != numLines_) _abort(CacheArray, "Wrong configuration.  Make sure numSets * associativity = Size/cacheLineSize\n");
    if(associativity_ < 1)                  _abort(CacheArray, "Associativity has to be greater than zero.\n");
}

}}

