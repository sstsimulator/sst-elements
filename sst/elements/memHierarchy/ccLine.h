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
    bool exclusiveSharerExists_;
    
    CCLine(Output* _dbg){
        d_ = _dbg;
        clear();
    }

    void setState(TCC_MESIState _newState) {
        d_->debug(C,L1,0, "Change States: Base Addr = %"PRIx64", Old State = %s, New State = %s\n",
                  baseAddr_, TccLineString[state_], TccLineString[_newState]);
        state_ = _newState;
    }
    
    void updateState() {
        if(numSharers_ == 0){
            setState(V);
            d_->debug(C,L4,0, "Updated TopCC State.  Sharer vector cleared.\n");
        }
    }
    
    void setBaseAddr(Addr _baseAddr){
        baseAddr_ = _baseAddr;
        assert(numSharers() == 0);
        assert(state_ == V);
    }
    
    Addr getBaseAddr(){ return baseAddr_; }
    
    void setExclusiveSharer(int _id) {
        assert(_id != -1);
        exclusiveSharerExists_ = true;
        addSharer(_id);
        assert(numSharers_ == 1);
        d_->debug(C,L2,0, "Setting Exclusive Sharer Flag..\n");
    }
    void clearExclusiveSharer(int _id) {
        exclusiveSharerExists_ = false;
        removeSharer(_id);
        d_->debug(C,L2,0,"Clearing Exclusive Sharer Flag..\n");
    }
    
    bool isValid(){ return getState() == V; }
    bool getExclusiveSharer() { return exclusiveSharerExists_; }
    bool valid() { return state_ == V; }
    bool inTransition() { return !valid(); }
    bool isSharer(int _id) { if(_id == -1) return false; return sharers_[_id]; }
    bool isShareless(){  return numSharers_ == 0; }
    bool exclusiveSharerExists(){ return (numSharers_ == 1) && (exclusiveSharerExists_); }
    TCC_MESIState getState() {return state_; }

    void removeAllSharers(){
        for(int i = 0; i < 128; i++){
            sharers_[i] = false;
        }
        assert(exclusiveSharerExists_ == false);
        numSharers_ = 0;
    }
    
    void assertSharers(){
        unsigned int count = 0;
        for(int i = 0; i < 128; i++){
            if(sharers_[i]) count++;
        }
        d_->debug(C,L2,0,"Num Sharers = %d, Actual Sharers = %d\n", numSharers_, count);
        assert(count == numSharers_);
    }
    
    void removeSharer(int _id){
        if(_id == -1) return;
        sharers_[_id] = false;
        numSharers_--;
        if(numSharers_ <= 0 || numSharers_ > 1) assert(exclusiveSharerExists_ == false);
        d_->debug(C,L2,0, "Removed Sharer: Num Sharers = %u\n", numSharers_);
    }
    
    uint numSharers(){ return numSharers_; }
    
    void addSharer(int _id){
        if(_id == -1) return;
        numSharers_++;
        sharers_[_id] = true;
        d_->debug(C,L2,0, "Added Sharer:  Num Sharers = %u\n", numSharers_);
    }

    void clear() {
        numSharers_ = 0;
        sharers_.reset();            
        exclusiveSharerExists_ = false;
        removeAllSharers();
        state_ = V;
        baseAddr_ = 0;
    }
};

#endif
