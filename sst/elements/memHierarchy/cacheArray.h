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
#include "sst/core/interfaces/memEvent.h"
#include <sst/core/serialization/element.h>
#include "sst/core/output.h"
#include "util.h"


using namespace std;
using namespace SST::Interfaces;

namespace SST { namespace MemHierarchy {

class ReplacementMgr;

class CacheArray {
public:
    class CacheLine {
    private:
        Output*         d_;
        BCC_MESIState   state_;
        Addr            baseAddr_;
        vector<uint8_t> data_;
        int             ackCount_;
        unsigned int    size_;
        MemEvent*       currentEvent_;
        unsigned int    userLock_;
        int             index_;

    public:

        bool eventsWaitingForLock_;

        CacheLine(Output* _dbg, unsigned int  _size) : d_(_dbg), state_(I), baseAddr_(0), ackCount_(0), size_(_size){
            data_.resize(size_/sizeof(uint8_t));
            for(vector<uint8_t>::iterator it = data_.begin(); it != data_.end(); it++) *it = 0;
            userLock_ = 0;
            index_ = -1;
            eventsWaitingForLock_ = false;
        }
	
        virtual ~CacheLine(){}
        BCC_MESIState getState() const { return state_; }
        bool waitingForAck(){return inTransition(); }
        bool inTransition(){
            return !unlocked();
        }
        //bool inPutTransition(){ return  (state_ == SI_PutAck || state_ == EI_PutAck || state_ == MI_PutAck || state_ == MS_PutAck); }
        bool valid() { return state_ != I; }
        bool unlocked() { return CacheLine::unlocked(state_);}
        bool locked() {return !unlocked();}
        MemEvent* getCurrentEvent() { return currentEvent_; }
        Addr getBaseAddr() { return baseAddr_; }
        void setBaseAddr(Addr _baseAddr) { baseAddr_ = _baseAddr; }
        unsigned int getLineSize(){ return size_; }
        vector<uint8_t>* getData(){ return &data_; }
        unsigned int getAckCount(){ assert(ackCount_ >= 0); return ackCount_; }
        void decAckCount(){ ackCount_--; if(ackCount_ == 0) setState(nextState[state_]); assert(ackCount_ >= 0);}
        void setAckCount(unsigned int _ackCount){ ackCount_ = _ackCount; }
        static bool unlocked(BCC_MESIState _state) { return _state == M || _state == S || _state == I || _state == E;}
        static bool inTransition(BCC_MESIState _state){ return !unlocked(_state);}
        void incLock(){ userLock_++; d_->debug(_L1_,"User Lock set on this cache line\n"); }
        void decLock(){ userLock_--; if(userLock_ == 0) d_->debug(_L1_,"User lock cleared on this cache line\n");}
        bool isLockedByUser(){ return (userLock_ > 0) ? true : false; }
        bool waitingForInvAck(){ return (state_ == SI || state_ == EI || state_ == MI || state_ == MS) ? true : false; }
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
        void setState(BCC_MESIState _newState){
            d_->debug(_L1_, "State change: bsAddr = %#016lx, oldSt = %s, newSt = %s\n", baseAddr_, BccLineString[state_], BccLineString[_newState]);
            state_ = _newState;
            if(inTransition(state_)) ackCount_++;
            if(_newState == I) assert(ackCount_ == 0);
            else assert(ackCount_ < 2);
            assert(userLock_ == 0);
        }
        
        void setIndex(int _index){ index_ = _index; }
        int  index(){ return index_; }
};
    
    typedef CacheArray::CacheLine CacheLine;

    /* Returns tag's ID if present, -1 otherwise.
     * If updateReplacement is set, runs the replacement algorithm on the updated block. 
     */
    virtual int find(Addr baseAddr, bool updateReplacement) = 0;

    /* Runs replacement scheme, returns tag ID of new pos and address of line to write back*/
    virtual unsigned int preReplace(Addr baseAddr) = 0;

    /* Write the new address in block. Only to be called
     * after preReplace
     */
    virtual void replace(Addr baseAddr, unsigned int candidate_id) = 0;
 
    vector<CacheLine*> lines_;
    
    bool isPowerOfTwo(unsigned int x){ return (x & (x - 1)) == 0; }
    Addr getLineSize(){ return lineSize_; }
    Addr toLineAddr(Addr addr);

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

    inline int find(Addr baseAddr, bool updateReplacement);
    inline unsigned int preReplace(Addr baseAddr);
    inline void replace(Addr baseAddr, unsigned int candidate_id);
    
private:
    Output* d_;
};

}}
#endif	/* CACHEARRAY_H */
