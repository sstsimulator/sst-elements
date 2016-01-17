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
 * File:   cacheArray.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef CACHEARRAY_H
#define CACHEARRAY_H

#include <vector>
#include <cstdlib>
#include <bitset>
#include "hash.h"
#include "memEvent.h"
#include <sst/core/serialization/element.h>
#include "sst/core/output.h"
#include "util.h"
#include "replacementManager.h"

using namespace std;

namespace SST { namespace MemHierarchy {

class ReplacementMgr;

class CacheArray {
public:

    /* Data line used for split coherence/data (i.e., directories ) */
    class DataLine {
    private:
        const uint32_t size_;
        const int index_;
        Output * dbg_;

        vector<uint8_t> data_;
        int dirIndex_;
    public:
        DataLine(unsigned int size, int index, Output * dbg) : size_(size), index_(index), dbg_(dbg), dirIndex_(-1) {
            data_.resize(size_/sizeof(uint8_t));
        }


        // Data getter/setter
        vector<uint8_t>* getData() { return &data_; }

        void setData(vector<uint8_t> data, MemEvent* ev) {
            if (ev->getSize() == size_) data_ = data;
            else {
                Addr offset = ev->getAddr() - ev->getBaseAddr();
                for(uint32_t i = 0; i < ev->getSize() ; i++ ) {
                    data_[offset + i] = ev->getPayload()[i];
                }
            }
        }

        // dirIndex getter/setter
        void setDirIndex(int dirIndex) { dirIndex_ = dirIndex; }
        int getDirIndex() { return dirIndex_; }
        
        // index getter
        int getIndex() { return index_; }
    };


    /* Cache line type - didn't bother splitting into different types because space overhead is small */
    class CacheLine {
    protected:
        const uint32_t      size_;
        const int           index_;
        Output *            dbg_;
        
        Addr                baseAddr_;
        State               state_;
        set<std::string>    sharers_;
        std::string         owner_;

        /* L1 specific */
        unsigned int userLock_;
        bool LLSCAtomic_;
        bool eventsWaitingForLock_;

        /* Dir specific - data stored separately */
        DataLine * dataLine_;

        /* Cache specific */
        vector<uint8_t> data_;

    public:
        CacheLine (unsigned int size, int index, Output * dbg, bool cache) : size_(size), index_(index), dbg_(dbg), baseAddr_(0), state_(I) {
            reset();
            if (cache) data_.resize(size_/sizeof(uint8_t));
        }
        
        virtual ~CacheLine() {}

        void reset() {
            state_ = I;
            sharers_.clear();
            owner_.clear();

            /* Dir specific */
            dataLine_ = NULL;

            /* L1 specific */
            userLock_               = 0;
            eventsWaitingForLock_   = false;
            LLSCAtomic_             = false;   
        }


        // Constant getters/setters
        unsigned int getSize() { return size_; }
        int getIndex() { return index_; }

        // Address getter & setter
        void setBaseAddr(Addr addr) { baseAddr_ = addr; }
        Addr getBaseAddr() { return baseAddr_; }

        // State getters & setter
        virtual void setState(State state) { 
            state_ = state; 
            if (state == I) {
                clearAtomics();
                sharers_.clear();
                owner_.clear();
            }
        }

        State getState() { return state_; }
        bool inTransition() { return (state_ != I) && (state_ != S) && (state_ != E) && (state_ != M) && (state_ != NP); }
        bool valid() { return state_ != I; }

        // Sharers getters & setter
        bool isShareless() { return sharers_.empty(); }
        set<std::string>* getSharers() { return &sharers_; }
        unsigned int numSharers() { return sharers_.size(); }
        
        bool isSharer(std::string name) { 
            if (name.empty()) return false; 
            return sharers_.find(name) != sharers_.end(); 
        }
        
        void removeSharer(std::string name) {
            if(name.empty()) return;
            if (sharers_.find(name) == sharers_.end()) dbg_->fatal(CALL_INFO, -1, "Error: cannot remove sharer '%s', not a current sharer. Addr = 0x%" PRIx64 "\n", name.c_str(), baseAddr_);
            sharers_.erase(name);        
        }
    
        void addSharer(std::string name) {
            if (name.empty()) return;
            sharers_.insert(name);
        }

        // Owner getters & setters
        void setOwner(std::string owner) { owner_ = owner; }
        std::string getOwner() { return owner_; }
        void clearOwner() { owner_.clear(); }
        bool ownerExists(){ return !owner_.empty(); }

        /****** L1 specific fields ******/
        // Bulk setter for atomics
        void clearAtomics() {
            if (userLock_ != 0) dbg_->fatal(CALL_INFO, -1, "Error: clearing cacheline but userLock is not 0. Address = 0x%" PRIx64 "\n", baseAddr_);
            if (eventsWaitingForLock_) dbg_->fatal(CALL_INFO, -1, "Error: clearing cacheline but eventsWaitingForLock is true. Address = 0x%" PRIx64 "\n", baseAddr_);
            atomicEnd();
        }
        
        // LLSCAtomic getters/setters
        void atomicStart(){ LLSCAtomic_ = true; }
        void atomicEnd() { LLSCAtomic_ = false; }
        bool isAtomic(){ return LLSCAtomic_; }

        // userLock getters/setters
        bool isLocked() { return (userLock_ > 0) ? true : false; }
        void incLock() { userLock_++; }
        void decLock() { userLock_--; }

        // eventsWaitingForLock getter/setter
        bool getEventsWaitingForLock(){ return eventsWaitingForLock_; }
        void setEventsWaitingForLock(bool _eventsWaiting){ eventsWaitingForLock_ = _eventsWaiting; }
        
        /***** Cache specific fields *****/
        // Data getter/setter
        vector<uint8_t>* getData() { return &data_; }

        void setData(vector<uint8_t> data, MemEvent* ev) {
            if (ev->getSize() == size_) data_ = data;
            else {
                Addr offset = ev->getAddr() - baseAddr_;
                for(uint32_t i = 0; i < ev->getSize() ; i++ ) {
                    data_[offset + i] = ev->getPayload()[i];
                }
            }
        }

        /***** Dir specific fields *****/
        // dataLine getter/setter
        void setDataLine(DataLine * line) { dataLine_ = line; }
        DataLine * getDataLine() { return dataLine_; }

    };

    typedef CacheArray::CacheLine CacheLine;
    typedef CacheArray::DataLine DataLine;

    /** Function returns the cacheline tag's ID if its valid (-1 if unvalid).
        If updateReplacement is set, the replacement stats are updated */
    virtual int find(Addr baseAddr, bool updateReplacement) = 0;

    /** Determine a replacement candidate using the replacement manager */
    virtual CacheLine * findReplacementCandidate(Addr baseAddr, bool cache) = 0;
    
    /** Replace cache line */
    virtual void replace(Addr baseAddr, unsigned int candidate_id, bool cache=true, unsigned int newLinkID=0) = 0;

    /** Determine if number is a power of 2 */
    bool isPowerOfTwo(unsigned int x) { return (x & (x - 1)) == 0;}
    
    /** Get line size.  Should not change at runtime */
    Addr getLineSize() { return lineSize_; }
    
    /** Drop block offset bits (ie. log2(lineSize) */
    Addr toLineAddr(Addr addr) { return (addr >> lineOffset_); }
    
    /** Destructor - Delete all cache line objects */
    virtual ~CacheArray() {
        for (unsigned int i = 0; i < lines_.size(); i++)
            delete lines_[i];
        delete replacementMgr_;
        delete hash_;
    }

    vector<CacheLine *> lines_;

private:
    void printConfiguration();
    void errorChecking();


protected:
    Output*         dbg_;
    unsigned int    numSets_;
    unsigned int    numLines_;
    unsigned int    associativity_;
    unsigned int    lineSize_;
    unsigned int    setMask_;
    unsigned int    lineOffset_;
    ReplacementMgr* replacementMgr_;
    HashFunction*   hash_;
    bool            sharersAware_;
    
    CacheArray(Output* dbg, unsigned int numLines, unsigned int associativity, unsigned int lineSize,
               ReplacementMgr* _replacementMgr, HashFunction* _hash, bool sharersAware, bool cache) : dbg_(dbg), 
               numLines_(numLines), associativity_(associativity), lineSize_(lineSize),
               replacementMgr_(_replacementMgr), hash_(_hash) {
        dbg_->debug(_INFO_,"--------------------------- Initializing [Set Associative Cache Array]... \n");
        numSets_    = numLines_ / associativity_;
        setMask_    = numSets_ - 1;
        lineOffset_ = log2Of(lineSize_);
        lines_.resize(numLines_);
    
        for (unsigned int i = 0; i < numLines_; i++) {
            lines_[i] = new CacheLine(lineSize_, i, dbg_, cache);
        }

        printConfiguration();
        errorChecking();
        sharersAware_ = sharersAware;
    }
};

/* Set-associative cache array */
class SetAssociativeArray : public CacheArray {
public:

    SetAssociativeArray(Output* dbg, unsigned int numLines, unsigned int lineSize, unsigned int associativity,
                        ReplacementMgr* rp, HashFunction* hf, bool sharersAware);

    int find(Addr baseAddr, bool updateReplacement);
    CacheLine * findReplacementCandidate(Addr baseAddr, bool cache);
    void replace(Addr baseAddr, unsigned int candidate_id, bool cache, unsigned int newLinkID);
    unsigned int preReplace(Addr baseAddr);
    void deallocate(unsigned int index);
    State * setStates;
    unsigned int * setSharers;
    bool * setOwned;
};

class DualSetAssociativeArray : public CacheArray {
public:
    DualSetAssociativeArray(Output * dbg, unsigned int lineSize, HashFunction * hf, bool sharersAware, unsigned int dirNumLines, unsigned int dirAssociativity, ReplacementMgr* dirRp, 
            unsigned int cacheNumLines, unsigned int cacheAssociativity, ReplacementMgr * cacheRp);

    int find(Addr baseAddr, bool updateReplacement);
    CacheLine * findReplacementCandidate(Addr baseAddr, bool cache);
    void replace(Addr baseAddr, unsigned int candidate_id, bool cache, unsigned int newLinkID);
    unsigned int preReplaceDir(Addr baseAddr);
    unsigned int preReplaceCache(Addr baseAddr);
    void deallocateCache(unsigned int index);
    
    vector<DataLine*> dataLines_;
    State * dirSetStates;
    unsigned int * dirSetSharers;
    bool * dirSetOwned;
    State * cacheSetStates;
    unsigned int * cacheSetSharers;
    bool * cacheSetOwned;

private:
    /* For our separate data cache */
    ReplacementMgr* cacheReplacementMgr_;
    unsigned int    cacheNumSets_;
    unsigned int    cacheNumLines_;
    unsigned int    cacheAssociativity_;
    unsigned int    cacheSetMask_;

};

}}
#endif	/* CACHEARRAY_H */
