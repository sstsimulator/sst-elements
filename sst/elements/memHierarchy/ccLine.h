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
    TCC_State     state_;
    Output*       d_;
    
    
public:
    bool ownerExists_;
    bool acksNeeded_;
    int  ownerId_;

    
    CCLine(Output* _dbg){
        d_ = _dbg;
        clear();
    }


    void setState(TCC_State _newState) {
        d_->debug(CALL_INFO,L4,0, "CCLine Changing State. Old State = %s, New State = %s\n", TccLineString[state_], TccLineString[_newState]);
        state_ = _newState;
    }
    
    void updateState(){ if(numSharers_ == 0)setState(V); }
    
    void setBaseAddr(Addr _baseAddr){
        baseAddr_ = _baseAddr;
        if (numSharers() != 0 || ownerExists_ || state_ != V) {
            d_->fatal(CALL_INFO,-1,"Error setting base addr 0x%" PRIx64 " in ccLine\n",_baseAddr);
        }
    }
    
    Addr getBaseAddr(){ return baseAddr_; }
    
    void setOwner(int _id) {
        if(numSharers_ != 0) {
            d_->fatal(CALL_INFO,-1,"Error: setting owner for 0x%" PRIx64 " but numSharers is %d\n", baseAddr_, numSharers_);
        }
        ownerId_ = _id;
        ownerExists_ = true;
        d_->debug(CALL_INFO,L4,0, "Owner set.\n");
    }
    
    void clearOwner() {
        ownerExists_ = false;
        if (numSharers_ != 0) {
            d_->fatal(CALL_INFO, -1, "Error: clearing owner for 0x%" PRIx64 " but numSharers is %d\n", baseAddr_, numSharers_);
        }
        ownerId_ = -1;
        d_->debug(CALL_INFO,L4,0,"Owner cleared.\n");
    }
    
    int getOwnerId(){
        if (ownerId_ == -1) {
            d_->fatal(CALL_INFO, -1, "Error: owner is not set in getOwnerId\n");
        }
        return ownerId_;
    }
    
    void setAcksNeeded(){ 
        if (acksNeeded_) {
            d_->fatal(CALL_INFO, -1, "Error: setting acks needed but acks needed already set. Addr = 0x%" PRIx64 "\n", baseAddr_);
        }
        acksNeeded_ = true; 
    }
    void clearAcksNeeded() { acksNeeded_ = false; }
    
    bool isValid(){ return getState() == V; }
    bool valid() { return state_ == V; }
    
    bool inStableState(){ return state_ == V; }
    bool inTransition(){ return !valid(); }
    bool isSharer(int _id) { if(_id == -1) return false; return sharers_[_id]; }
    bool isShareless(){ return numSharers_ == 0; }
    bool ownerExists(){ return ownerExists_; }
 
    TCC_State getState() {return state_; }

    void removeAllSharers(){
        for(int i = 0; i < 128; i++){
            sharers_[i] = false;
        }
        if (ownerExists_) {
            d_->fatal(CALL_INFO, -1, "Error: removing sharers but block is owned. Addr = 0x%" PRIx64 "\n", baseAddr_);
        }
        numSharers_ = 0;
    }
    
    void assertSharers(){
        unsigned int count = 0;
        for(int i = 0; i < 128; i++){
            if(sharers_[i]) count++;
        }
        if (count != numSharers_) {
            d_->fatal(CALL_INFO, -1, "Error: count sharers failed. Counted %u, numSharers is %u. Addr = 0x%" PRIx64 "\n", count, numSharers_, baseAddr_);
        }
    }
    
    void removeSharer(int _id){
        if(_id == -1) return;
        if (numSharers_ == 0) {
            d_->fatal(CALL_INFO, -1, "Error: no sharers to remove. Addr = 0x%" PRIx64 ". Sharer to be removed = %d\n", baseAddr_, _id);
        }
        if (sharers_[_id] != true) {
            d_->fatal(CALL_INFO, -1, "Error: cannot remove sharer id %d, not a current sharer. Addr = 0x%" PRIx64 "\n", _id, baseAddr_);
        }
        
        sharers_[_id] = false;
        numSharers_--;
        
        d_->debug(CALL_INFO,L4,0, "Removed sharer. Number of sharers sharers = %u\n", numSharers_);
        
        updateState();

    }
    
    uint numSharers(){ return numSharers_; }
    
    void addSharer(int _id){
        if(_id == -1) return;
        numSharers_++;
        sharers_[_id] = true;
        d_->debug(CALL_INFO,L4,0, "Added sharer.  Number of sharers = %u\n", numSharers_);
        if (ownerExists_) {
            d_->fatal(CALL_INFO, -1, "Error: cannot add sharer, owner exists. Addr = 0x%" PRIx64 "\n", baseAddr_);
            
        }

    }

    void clear() {
        sharers_.reset();
        clearAcksNeeded();
        ownerExists_  = false;
        removeAllSharers();
        state_        = V;
        baseAddr_     = 0;
        numSharers_   = 0;
        ownerId_      = -1;
    }
};

#endif
