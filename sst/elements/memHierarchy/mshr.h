/* 
 * File:   mshr.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef SST_mshr_h
#define SST_mshr_h

#include "util.h"

namespace SST { namespace MemHierarchy {

/*--------------------------------------------------------------------------------------------------------
 *  MSHR Methods
 *--------------------------------------------------------------------------------------------------------*/
Cache::MSHR::MSHR(Cache* _cache, uint _maxSize): cache_(_cache), size_(0), maxSize_(_maxSize){}


const vector<mshrType*> Cache::MSHR::lookup(Addr baseAddr){
    mshrTable::iterator it = map_.find(baseAddr);
    assert(it != map_.end());
    vector<mshrType*> res = it->second;
    return res;
}

bool Cache::MSHR::insertPointer(Addr keyAddr, Addr pointerAddr){
    return insert(keyAddr, pointerAddr);
}

bool Cache::MSHR::insert(Addr baseAddr, MemEvent* event){
    cache_->d_->debug(C,L5,0, "MSHR Event Inserted: Key Addr = %#016llx, Event Addr = %#016llx, Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", baseAddr, (uint64_t)event->getAddr(), CommandString[event->getCmd()], size_, map_[baseAddr].size());
    return insert(baseAddr, new mshrType(event));
}

bool Cache::MSHR::insert(Addr baseAddr, Addr pointer){
    return insert(baseAddr, new mshrType(pointer));
}

bool Cache::MSHR::insert(Addr baseAddr, mshrType* mshrEntry){
    map_[baseAddr].push_back(mshrEntry);
    size_++;
    addrLastInserted_ = baseAddr;
    return true;
}

bool Cache::MSHR::insert(Addr baseAddr, vector<mshrType*> events){
    if(events.empty()) return false;
    mshrTable::iterator it = map_.find(baseAddr);
    
    if(it != map_.end()) it->second.insert(it->second.end(), events.begin(), events.end());
    else {
        map_[baseAddr] = events;
        size_ = 0;
    }
    //size_ = size_ + events.size();
    //cache_->d_->debug(C,L5,0, "MSHR Inserted All Events\n");
    //cache_->d_->debug(C,L5,0, "MSHR Inserted All Events: Key Addr = %#016lllx, Event Addr = %#016lllx, Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", (uint64_t)baseAddr, (uint64_t)events.front()->getAddr(), CommandString[events.front()->getCmd()], size_, map_[baseAddr].size());
    addrLastInserted_ = baseAddr;
    return true;
}

vector<mshrType*> Cache::MSHR::remove(Addr baseAddr){
    mshrTable::iterator it = map_.find(baseAddr);
    assert(it != map_.end());
    vector<mshrType*> res = it->second;
    map_.erase(it);
    //size_ = size_ - res.size(); assert(size_ >= 0);
    //cache_->d_->debug(C,L5,0, "MSHR Removed All Events\n");
    //cache_->d_->debug(C,L5,0, "MSHR Removed All Events: Key Addr = %#016lllx, Entry Size = %lu\n", (uint64_t)baseAddr, res.size());
    return res;
}

bool Cache::MSHR::isHit(Addr baseAddr){ return map_.find(baseAddr) != map_.end(); }

/* Function checks if incoming request should be stalled.  If there is mshr hit the
 * a pending instruction for the same address exists so new request should stall till
 * the this requests finishes */
bool Cache::MSHR::isHitAndStallNeeded(Addr baseAddr, Command cmd){
    mshrTable::iterator it = map_.find(baseAddr);
    /*GetX does not stall if there is there is an Inv at the front of MSHR queue 
     * since cache line can be user-locked.  If user-locked, Invalidates can pile up in the MSHR
     * but GetX still needs to continue since Inv won't get re-activated until user-lock is released */
    if(it == map_.end() || it->second.empty()) return false;
    if(it->second.front()->elem.type() == typeid(Addr)) return false;  //front-of-the-vector event cannot be of pointer type
    MemEvent* frontEvent = boost::get<MemEvent*>(it->second.front()->elem);
    if(cmd == GetX) return (it != map_.end() &&  frontEvent->getCmd() != Inv);  
    else{
        cache_->d_->debug(_WARNING_, "Blocking Request: MSHR entry exists and waiting to complete. TopOfQueue Request Cmd: %s\n", CommandString[frontEvent->getCmd()]);
        return true;
    }
}

struct MSHREntryCompare {
    enum Type {Event, Pointer};
    MSHREntryCompare( mshrType* _m ) : m(_m) {
        if(m->elem.type() == typeid(MemEvent*)) type = Event;
        else type = Pointer;
    }
    bool operator() (mshrType* &n){
        if(type == Event){
            if(n->elem.type() == typeid(MemEvent*)) return boost::get<MemEvent*>(m->elem) == boost::get<MemEvent*>(n->elem);
            return false;
        }
        else{
            if(n->elem.type() == typeid(Addr)) return boost::get<Addr>(m->elem) == boost::get<Addr>(n->elem);
            return false;
        }
    }
    mshrType* m;
    Type type;
};


void Cache::MSHR::removeElement(Addr baseAddr, MemEvent* event){
    removeElement(baseAddr, new mshrType(event));
}

void Cache::MSHR::removeElement(Addr baseAddr, Addr pointer){
    removeElement(baseAddr, new mshrType(pointer));
}

void Cache::MSHR::removeElement(Addr baseAddr, mshrType* mshrEntry){

    mshrTable::iterator it = map_.find(baseAddr);
    if(it == map_.end()) return;    
    cache_->d_->debug(C,0,0, "MSHR Entry size = %lu\n", it->second.size());
    vector<mshrType*>& res = it->second;
    vector<mshrType*>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(mshrEntry));
    
    if(itv == res.end()) return;
    res.erase(std::remove_if(res.begin(), res.end(), MSHREntryCompare(mshrEntry)), res.end());

    if(res.empty()) map_.erase(it);
    size_--; assert(size_ >= 0);
    cache_->d_->debug(C,0,0, "MSHR Removed Event\n");
}

/*
void Cache::MSHR::printEntry(Addr baseAddr){
    mshrTable::iterator it = map_.find(baseAddr);
    if(it == map_.end()) return;
    int i = 0;
    for(vector<MemEvent*>::iterator it = map_[baseAddr].begin(); it != map_[baseAddr].end(); ++it, ++i) {
        MemEvent* event = (MemEvent*)(*it);
        cache_->d_->debug(C,L5,0, "Entry %i:  Key Addr: %#016lllx, Event Addr: %#016lllx, Event Cmd = %s\n", i, (uint64_t)baseAddr, (uint64_t)event->getAddr(), CommandString[event->getCmd()]);
    }
}

void Cache::MSHR::printEntry2(vector<MemEvent*> events){
    int i = 0;
    for(vector<MemEvent*>::iterator it = events.begin(); it != events.end(); ++it, ++i) {
        MemEvent* event = (MemEvent*)(*it);
        cache_->d_->debug(C,L5,0, "Entry %i:  Event Addr: %#016lllx, Event Cmd = %s, Dst: %s\n", i, (uint64_t)event->getAddr(), CommandString[event->getCmd()], event->getDst().c_str());
    }
}
*/
}};



#endif
