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
 * File:   cacheArray.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef CACHEARRAY_H
#define CACHEARRAY_H

#include <vector>
#include <cstdlib>
#include "hash.h"
#include "memEvent.h"
#include <sst/core/serialization/element.h>
#include "sst/core/output.h"
#include "util.h"


using namespace std;

namespace SST { namespace MemHierarchy {

class ReplacementMgr;

class CacheArray {
public:
    
    /** Function returns the cacheline tag's ID if its valid (-1 if unvalid).
        If updateReplacement is set, the replacement stats are updated */
    virtual int find(Addr baseAddr, bool updateReplacement) = 0;

    /** Runs replacement scheme, returns tag ID of new pos and address of line to write back */
    virtual unsigned int preReplace(Addr baseAddr) = 0;

    /** Replace cache line */
    virtual void replace(Addr baseAddr, unsigned int candidate_id) = 0;
    
    /** Determine if number is a power of 2 */
    bool isPowerOfTwo(unsigned int x){ return (x & (x - 1)) == 0;}
    
    /** Get line size.  Should not change at runtime */
    Addr getLineSize(){ return lineSize_; }
    
    /** Drop block offset bits (ie. log2(lineSize) */
    Addr toLineAddr(Addr addr){ return (addr >> lineOffset_); }
    
    /** Destructor - Delete all cache line objects */
    virtual ~CacheArray(){
        for (unsigned int i = 0; i < lines_.size(); i++)
            delete lines_[i];
    }

    class CacheLine {
    private:
        Output*         d_;
        State           state_;
        Addr            baseAddr_;
        vector<uint8_t> data_;
        const uint32_t  size_;
        const int       index_;
        
        unsigned int    userLock_;
        bool            LLSCAtomic_;
        bool            eventsWaitingForLock_;
    
    public:
        /** Constructor */
        CacheLine(Output* _dbg, unsigned int _size, int _index) :
                 d_(_dbg), state_(I), baseAddr_(0), size_(_size), index_(_index){
            data_.resize(size_/sizeof(uint8_t));
            reset();
        }
        
        void atomicStart(){ LLSCAtomic_ = true; }
        void atomicEnd(){ LLSCAtomic_ = false; }
        bool isAtomic(){ return LLSCAtomic_; }
        
        bool isLocked(){ return (userLock_ > 0) ? true : false; }

        void incLock(){
            userLock_++;
            d_->debug(_L8_,"User-level lock set on this cache line\n");
        }
        
        void decLock(){
            userLock_--;
            if(userLock_ == 0) d_->debug(_L8_,"User lock cleared on this cache line\n");
        }
        
        vector<uint8_t>* getData(){ return &data_; }
        
        void setData(vector<uint8_t> _data, MemEvent* ev){
            /* Update whole cache line */
            if (ev->getSize() == size_) data_ = _data;
        
            /* Update a portion of the block */
            else {
                assert(ev->getAddr() >= baseAddr_);
                Addr offset = ev->getAddr() - baseAddr_;
                for(uint32_t i = 0; i < ev->getSize() ; i++ ) {
                    data_[offset + i] = ev->getPayload()[i];
                }
            }
        }
        
        State getState() const { return state_; }
        
        void setState(State _newState){
            d_->debug(_L6_, "Changing states: Old state = %s, New State = %s\n", BccLineString[state_], BccLineString[_newState]);
            state_ = _newState;
            if(state_ == I) clear();
        }
        
        bool valid() { return state_ != I; }
        bool inStableState(){ return inStableState(state_);}
        bool inTransition(){ return !inStableState();}
        
        Addr getBaseAddr() { return baseAddr_; }
        void setBaseAddr(Addr _baseAddr) { baseAddr_ = _baseAddr; }
        
        static bool inStableState(State _state) { return _state == M || _state == S || _state == I || _state == E;}
        static bool inTransition(State _state){ return !inStableState(_state);}

        
        bool getEventsWaitingForLock(){ return eventsWaitingForLock_; }
        void setEventsWaitingForLock(bool _eventsWaiting){ eventsWaitingForLock_ = _eventsWaiting; }

        int getIndex(){ return index_; }
        unsigned int getLineSize(){ return size_; }
        
        void clear(){
            vector<uint8_t>::iterator it;
            for(it = data_.begin(); it != data_.end(); it++){ *it = 0; }
            assert(state_ == I);
            assert(userLock_ == 0);
            assert(eventsWaitingForLock_ == false);
            atomicEnd();
        }
        
        void reset(){
            vector<uint8_t>::iterator it;
            for(it = data_.begin(); it != data_.end(); it++){ *it = 0; }
            state_                  = I;
            userLock_               = 0;
            eventsWaitingForLock_   = false;
            LLSCAtomic_             = false;
        }

    };
    
    typedef CacheArray::CacheLine CacheLine;
    vector<CacheLine*> lines_;

    
private:
    void pMembers();
    void errorChecking();


protected:
    Output*         d_;
    unsigned int    cacheSize_;
    unsigned int    numSets_;
    unsigned int    numLines_;
    unsigned int    associativity_;
    unsigned int    lineSize_;
    unsigned int    setMask_;
    unsigned int    lineOffset_;
    ReplacementMgr* replacementMgr_;
    HashFunction*   hash_;
    bool            sharersAware_;
    
    CacheArray(Output* _dbg, unsigned int _cacheSize, unsigned int _numLines, unsigned int _associativity, unsigned int _lineSize,
               ReplacementMgr* _replacementMgr, HashFunction* _hash, bool _sharersAware) : d_(_dbg), cacheSize_(_cacheSize),
               numLines_(_numLines), associativity_(_associativity), lineSize_(_lineSize),
               replacementMgr_(_replacementMgr), hash_(_hash) {
        d_->debug(_INFO_,"--------------------------- Initializing [Set Associative Cache Array]... \n");
        numSets_    = numLines_ / associativity_;
        setMask_    = numSets_ - 1;
        lineOffset_ = log2Of(lineSize_);
        lines_.resize(numLines_);

        for(unsigned int i = 0; i < numLines_; i++){
            lines_[i] = new CacheLine(_dbg, lineSize_, i);
        }
        
        pMembers();
        errorChecking();
        sharersAware_ = _sharersAware;
    }
};

/* Set-associative cache array */
class SetAssociativeArray : public CacheArray {
public:

    SetAssociativeArray(Output* _dbg, unsigned int _cacheSize, unsigned int _lineSize, unsigned int _associativity,
                        ReplacementMgr* _rp, HashFunction* _hf, bool _sharersAware);

    int find(Addr baseAddr, bool updateReplacement);
    unsigned int preReplace(Addr baseAddr);
    void replace(Addr baseAddr, unsigned int candidate_id);
    State * setStates;
};

}}
#endif	/* CACHEARRAY_H */
