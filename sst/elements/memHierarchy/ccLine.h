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
 * File:   ccLine.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef SST_CCLine_h
#define SST_CCLine_h

class CCLine {
    typedef unsigned int uint;
protected:
    uint          numSharers_;
    bitset<128>   sharers_;
    Addr          baseAddr_;
    TCC_MESIState state_;
    Output*       d_;
    
    
public:
    bool ownerExists_;
    bool acksNeeded_;
    int  ownerId_;

    
    CCLine(Output* _dbg){
        d_ = _dbg;
        ownerId_ = -1;
        clear();
    }


    void setState(TCC_MESIState _newState) {
        d_->debug(C,L4,0, "CCLine Changing State. Old State = %s, New State = %s\n", TccLineString[state_], TccLineString[_newState]);
        state_ = _newState;
    }
    
    void updateState() {
        if(numSharers_ == 0){
            setState(V);
        }
    }
    
    void setBaseAddr(Addr _baseAddr){
        baseAddr_ = _baseAddr;
        assert(numSharers() == 0);
        assert(state_ == V);
    }
    
    Addr getBaseAddr(){ return baseAddr_; }
    
    void setOwner(int _id) {
        assert(_id != -1);
        assert(numSharers_ == 0);
        ownerId_ = _id;
        ownerExists_ = true;
        d_->debug(C,L4,0, "Owner set.\n");
    }
    
    void clearOwner() {
        ownerExists_ = false;
        assert(numSharers_ == 0);
        ownerId_ = -1;
        d_->debug(C,L4,0,"Owner cleared.\n");
    }
    
    int getOwnerId(){
        assert(ownerId_ != -1);
        return ownerId_;
    }
    
    void setAcksNeeded(){ assert(acksNeeded_ == false); acksNeeded_ = true; }
    void clearAcksNeeded() { acksNeeded_ = false; }
    
    bool isValid(){ return getState() == V; }
    bool valid() { return state_ == V; }
    
    bool inStableState(){ return state_ == V; }
    bool inTransition(){ return !valid(); }
    bool isSharer(int _id) { if(_id == -1) return false; return sharers_[_id]; }
    bool isShareless(){  return numSharers_ == 0; }
    bool ownerExists(){
        if(ownerExists_) assert(ownerId_ != -1);
        return ownerExists_;
    }
    TCC_MESIState getState() {return state_; }

    void removeAllSharers(){
        for(int i = 0; i < 128; i++){
            sharers_[i] = false;
        }
        assert(ownerExists_ == false);
        numSharers_ = 0;
    }
    
    void assertSharers(){
        unsigned int count = 0;
        for(int i = 0; i < 128; i++){
            if(sharers_[i]) count++;
        }
        assert(count == numSharers_);
    }
    
    void removeSharer(int _id){
        if(_id == -1) return;
        assert(numSharers_ > 0);
        assert(sharers_[_id] == true);
        
        sharers_[_id] = false;
        numSharers_--;
        
        if(numSharers_ > 0) assert(ownerExists_ == false);
        d_->debug(C,L4,0, "Removed sharer. Number of sharers sharers = %u\n", numSharers_);
        
        updateState();
        assertSharers();

    }
    
    uint numSharers(){ return numSharers_; }
    
    void addSharer(int _id){
        if(_id == -1) return;
        numSharers_++;
        sharers_[_id] = true;
        d_->debug(C,L4,0, "Added sharer.  Number of sharers = %u\n", numSharers_);
        assertSharers();
    }

    void clear() {
        sharers_.reset();
        clearAcksNeeded();
        ownerExists_  = false;
        removeAllSharers();
        state_        = V;
        baseAddr_     = 0;
        numSharers_   = 0;
    }
};

#endif
