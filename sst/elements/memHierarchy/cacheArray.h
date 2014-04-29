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
    bool isPowerOfTwo(unsigned int x){
        return (x & (x - 1)) == 0;
    }
    
    /** Get line size.  Should not change at runtime */
    Addr getLineSize(){ return lineSize_; }
    
    /** Drop block offset bits (ie. log2(lineSize) */
    Addr toLineAddr(Addr addr){
        return (addr >> lineOffset_);
    }
    
    /** Destructor - Delete all cache line objects */
    virtual ~CacheArray(){
        for (unsigned int i = 0; i < lines_.size(); i++)
            delete lines_[i];
    }

    class CacheLine {
    private:
        Output*         d_;
        BCC_MESIState   state_;
        Addr            baseAddr_;
        vector<uint8_t> data_;
        unsigned int    size_;
        unsigned int    userLock_;
        int             index_;

    public:
        bool eventsWaitingForLock_;
        
         /** Constructor */
        CacheLine(Output* _dbg, unsigned int _size) :
                 d_(_dbg), state_(I), baseAddr_(0), size_(_size){
            data_.resize(size_/sizeof(uint8_t));
            for(vector<uint8_t>::iterator it = data_.begin(); it != data_.end(); it++)
                *it = 0;
            userLock_ = 0;
            index_ = -1;
            eventsWaitingForLock_ = false;
        }
        

        bool isLockedByUser(){ return (userLock_ > 0) ? true : false; }

        void incLock(){
            userLock_++;
            d_->debug(_L1_,"User Lock set on this cache line\n");
        }
        
        void decLock(){
            userLock_--;
            if(userLock_ == 0) d_->debug(_L1_,"User lock cleared on this cache line\n");
        }
        
        vector<uint8_t>* getData(){ return &data_; }
        
        void setData(vector<uint8_t> _data, MemEvent* ev){
            if (ev->getSize() == size_ || ev->getCmd() == GetSEx) {
                std::copy(_data.begin(), _data.end(), this->data_.begin());
                data_ = _data;
                printData(d_, "Cache line write", &_data);
            } else {
                // Update a portion of the block
                printData(d_, "Partial cache line write", &ev->getPayload());
                Addr offset = (ev->getAddr() <= baseAddr_) ? 0 : ev->getAddr() - baseAddr_;

                Addr payloadoffset = (ev->getAddr() >= baseAddr_) ? 0 : baseAddr_ - ev->getAddr();
                assert(payloadoffset == 0);
                for ( uint32_t i = 0 ; i < std::min(size_,ev->getSize()) ; i++ ) {
                    data_[offset + i] = ev->getPayload()[payloadoffset + i];
                }
            }
        }
        
        BCC_MESIState getState() const { return state_; }
        void updateState(){ setState(nextState[state_]); }
        void setState(BCC_MESIState _newState){
            d_->debug(_L1_, "State change: bsAddr = %"PRIx64", oldSt = %s, newSt = %s\n", baseAddr_, BccLineString[state_], BccLineString[_newState]);
            state_ = _newState;
            assert(userLock_ == 0);
        }
        
        bool valid() { return state_ != I; }
        bool inTransition(){ return !unlocked();}
        static bool inTransition(BCC_MESIState _state){ return !unlocked(_state);}

        Addr getBaseAddr() { return baseAddr_; }
        void setBaseAddr(Addr _baseAddr) { baseAddr_ = _baseAddr; }
        
        bool locked() {return !unlocked();}
        bool unlocked() { return CacheLine::unlocked(state_);}
        static bool unlocked(BCC_MESIState _state) { return _state == M || _state == S || _state == I || _state == E;}
        
        
        void setIndex(int _index){ index_ = _index; }
        int  index(){ return index_; }

        unsigned int getLineSize(){ return size_; }

    };
    
    typedef CacheArray::CacheLine CacheLine;
    vector<CacheLine*> lines_;

    
private:
    void pMembers();
    void errorChecking();


protected:
    Output* d_;
    unsigned int cacheSize_; 
    unsigned int numSets_;
    unsigned int numLines_;
    unsigned int associativity_;
    unsigned int lineSize_;
    unsigned int setMask_;
    unsigned int lineOffset_;
    ReplacementMgr* replacementMgr_;
    HashFunction*   hash_;
    bool sharersAware_;
    
    CacheArray(Output* _dbg, unsigned int _cacheSize, unsigned int _numLines, unsigned int _associativity, unsigned int _lineSize,
               ReplacementMgr* _replacementMgr, HashFunction* _hash, bool _sharersAware) : d_(_dbg), cacheSize_(_cacheSize),
               numLines_(_numLines), associativity_(_associativity), lineSize_(_lineSize),
               replacementMgr_(_replacementMgr), hash_(_hash) {
        d_->debug(_INFO_,"--------------------------- Initializing [Set Associative Cache Array]... \n");
        numSets_ = numLines_ / associativity_;
        setMask_ = numSets_ - 1;
        lines_.resize(numLines_);
        lineOffset_ = log2Of(lineSize_);
        for(unsigned int i = 0; i < numLines_; i++){
            lines_[i] = new CacheLine(_dbg, lineSize_);
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

};

}}
#endif	/* CACHEARRAY_H */
