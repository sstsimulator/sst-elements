// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_LINETYPES_H
#define MEMHIERARCHY_LINETYPES_H

#include <vector>

#include <sst/core/output.h>

#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/replacementManager.h"

using namespace std;

namespace SST { namespace MemHierarchy {

/*
 * Line types
 * Required API:
 * - reset() to reset to invalid state
 * - getString() for debug
 * - getAddr() for identifying a line
 * - getReplacementInfo() for returning the information that a replacement policy might need
 * - allocated() to determine whether the line is currently allocated/valid
 */



/* Line which only holds coherence state - no data */
class DirectoryLine {
    private:
        const unsigned int index_;
        Addr addr_;
        State state_;
        std::set<std::string> sharers_;
        std::string owner_;
        uint64_t lastSendTimestamp_;
        CoherenceReplacementInfo * info_;
        bool wasPrefetch_;

    public:
        DirectoryLine(uint32_t size, unsigned int index) : index_(index) {
            info_ = new CoherenceReplacementInfo(index, I, false, false);
            reset();
        }
        virtual ~DirectoryLine() { }

        void reset() {
            addr_ = NO_ADDR;
            state_ = I;
            sharers_.clear();
            owner_ = "";
            lastSendTimestamp_ = 0;
            wasPrefetch_ = false;
            info_->reset();
        }

        // Index
        unsigned int getIndex() { return index_; }

        // Addr
        Addr getAddr() { return addr_; }
        void setAddr(Addr addr) { addr_ = addr; }

        // State
        State getState() { return state_; }
        void setState(State state) { state_ = state; }

        // Sharers
        set<std::string>* getSharers() { return &sharers_; }
        bool isSharer(std::string shr) {   return sharers_.find(shr) != sharers_.end(); }
        size_t numSharers() { return sharers_.size(); }
        bool hasSharers() { return !sharers_.empty(); }
        bool hasOtherSharers(std::string shr) { return !(sharers_.empty() || (sharers_.size() == 1 && sharers_.find(shr) != sharers_.end())); }
        void addSharer(std::string shr) {
            sharers_.insert(shr);
            info_->setShared(true);
        }
        void removeSharer(std::string shr) {
            sharers_.erase(shr);
            info_->setShared(!sharers_.empty());
        }

        // Owner
        std::string getOwner() { return owner_; }
        bool hasOwner() { return !owner_.empty(); }
        void setOwner(std::string owner) {
            owner_ = owner;
            info_->setOwned(true);
        }
        void removeOwner() {
            owner_.clear();
            info_->setOwned(false);
        }

        // Timestamp
        uint64_t getTimestamp() { return lastSendTimestamp_; }
        void setTimestamp(uint64_t timestamp) { lastSendTimestamp_ = timestamp; }

        // Prefetch
        bool getPrefetch() { return wasPrefetch_; }
        void setPrefetch(bool prefetch) { wasPrefetch_ = prefetch; }


        // Replacement
        ReplacementInfo* getReplacementInfo() { return info_; }

        // Validity
        bool allocated() { return state_ != I; }

        // String-ify for debugging
        std::string getString() {
            std::ostringstream str;
            str << "O: " << (owner_.empty() ? "-" : owner_);
            str << " S: [";
            for (std::set<std::string>::iterator it = sharers_.begin(); it != sharers_.end(); it++) {
                if (it != sharers_.begin()) str << ",";
                str << *it;
            }
            str << "]";
            return str.str();
        }

        DirectoryLine() : index_(0) {}
        void serialize_order(SST::Core::Serialization::serializer& ser) {
            SST_SER(const_cast<unsigned int&>(index_));
            SST_SER(addr_);
            SST_SER(state_);
            SST_SER(sharers_);
            SST_SER(owner_);
            SST_SER(lastSendTimestamp_);
            SST_SER(info_);
            SST_SER(wasPrefetch_);
        }
};

/* Line which only holds data - and points to a directory line */
class DataLine {
    private:
        const unsigned int index_;
        Addr addr_;
        vector<uint8_t> data_;
        DirectoryLine* tag_;
        CoherenceReplacementInfo* info_;
    public:
        DataLine(uint8_t size, unsigned int index) : index_(index) {
            data_.resize(size);
            info_ = new CoherenceReplacementInfo(index, I, false, false);
            reset();
        }
        virtual ~DataLine() { }

        void reset() {
            addr_ = NO_ADDR;
            tag_ = nullptr;
            info_->reset();
        }

        // Index
        unsigned int getIndex() { return index_; }

        // Addr
        Addr getAddr() { return addr_; }
        void setAddr(Addr addr) { addr_ = addr; }

        // Valid
        State getState() { return tag_ ? tag_->getState() : I; }
        void setTag(DirectoryLine* tag) {
            tag_ = tag;
        }
        DirectoryLine* getTag() { return tag_; }

        // Data
        vector<uint8_t>* getData() { return &data_; }
        void setData(vector<uint8_t> data, uint32_t offset) {
            std::copy(data.begin(), data.end(), data_.begin() + offset);
        }

        // Replacement
        ReplacementInfo* getReplacementInfo() { return (tag_ != nullptr ? tag_->getReplacementInfo() : info_); }

        // Validity
        bool allocated() { return tag_ != nullptr; }

        // String-ify for debugging
        std::string getString() {
            return (tag_ ? "Valid" : "Invalid");
        }

        DataLine() : index_(0) {}
        void serialize_order(SST::Core::Serialization::serializer& ser) {
            SST_SER(const_cast<unsigned int&>(index_));
            SST_SER(addr_);
            SST_SER(data_);
            SST_SER(info_);
        }
};

/* Base class for a line that contains both data & coherence state */
class CacheLine : public SST::Core::Serialization::serializable {
    protected:
        const unsigned int index_;
        Addr addr_;
        State state_;
        vector<uint8_t> data_;

        // Timing
        uint64_t lastSendTimestamp_;

        // Statistics
        bool wasPrefetch_;

        virtual void updateReplacement() = 0;
    public:
        CacheLine(uint32_t size, unsigned int index) : index_(index) {
            reset();
            data_.resize(size);
        }
        virtual ~CacheLine() { }

        void reset() {
            addr_ = NO_ADDR;
            state_ = I;
            lastSendTimestamp_ = 0;
            wasPrefetch_ = false;
        }

        // Index
        unsigned int getIndex() { return index_; }

        // Addr
        Addr getAddr() { return addr_; }
        void setAddr(Addr addr) { addr_ = addr; }

        // State
        State getState() { return state_; }
        void setState(State state) { state_ = state; updateReplacement(); }

        // Data
        vector<uint8_t>* getData() { return &data_; }
        void setData(vector<uint8_t> in, uint32_t offset) {
            std::copy(in.begin(), in.end(), std::next(data_.begin(), offset));
        }

        // Timestamp
        uint64_t getTimestamp() { return lastSendTimestamp_; }
        void setTimestamp(uint64_t timestamp) { lastSendTimestamp_ = timestamp; }

        // Prefetch
        bool getPrefetch() { return wasPrefetch_; }
        void setPrefetch(bool prefetch) { wasPrefetch_ = prefetch; }

        virtual ReplacementInfo* getReplacementInfo() = 0;

        // Validity
        bool allocated() { return state_ != I; }

        // String-ify for debugging
        std::string getString() {
            std::ostringstream str;
            str << std::hex << "0x" << addr_;
            str << " State: " << StateString[state_];
            return str.str();
        }

        CacheLine() : index_(0) {}
        virtual void serialize_order(SST::Core::Serialization::serializer& ser) override {
            SST_SER(const_cast<unsigned int&>(index_));
            SST_SER(addr_);
            SST_SER(state_);
            SST_SER(data_);
            SST_SER(lastSendTimestamp_);
            SST_SER(wasPrefetch_);
        }
        ImplementVirtualSerializable(SST::MemHierarchy::CacheLine);
};

/* With atomic/lock flags for L1 caches */
class L1CacheLine : public CacheLine {
    private:
        bool LLSC_;           /* True if LL has been issued and no intervening accesses have occurred */
        Cycle_t LLSCTime_;    /* For caches that guarantee forward progress on LLSC, LLSC temporarily locks the line. This is the time that the LLSC lock will be released */
        std::set<uint32_t> LLSCTidBuf_; /* Track hardware thread IDs that have LL'd this line*/
                                        /* This is stored in the line rather than in a separate cache-side map to make lookup & maintenance faster (esp. on non-atomic accesses) */
                                        /* TODO add ability to limit outstanding LLSCs per thread and/or overall (requires a cache-side structure to track global cache state) */
        unsigned int userLock_;     /* Count number of lock operations to the line */
        bool eventsWaitingForLock_; /* Number of events in the queue waiting for the lock */
        ReplacementInfo * info_;     /* Replacement info - depends on replacement algorithm */
    protected:
        void updateReplacement() override { info_->setState(state_); }
    public:
        L1CacheLine(uint32_t size, unsigned int index) : CacheLine(size, index), LLSC_(false), LLSCTime_(0), userLock_(0), eventsWaitingForLock_(false) {
            info_ = new ReplacementInfo(index, I);
        }
        virtual ~L1CacheLine() {
            delete info_;
        }

        void reset() {
            CacheLine::reset();
            LLSC_ = false;
            LLSCTime_ = 0;
            LLSCTidBuf_.clear();
            userLock_ = 0;
            eventsWaitingForLock_ = false;
            info_->setState(getState());
        }

        // LLSC
        void atomicStart(Cycle_t time, uint32_t tid) {
            if (!LLSC_) {
                LLSC_ = true;
                LLSCTime_ = time; // To reduce starvation, this does not update on subsequent hits to an LL'd line
            }
            LLSCTidBuf_.insert(tid);
        }

        void atomicEnd() {
            if(LLSC_) {
                LLSC_ = false;
                LLSCTime_ = 0;
                LLSCTidBuf_.clear();
            }
        }

        bool isAtomic(uint32_t tid) { return LLSC_ && (LLSCTidBuf_.find(tid) != LLSCTidBuf_.end()); }

        // Lock
        bool isLocked(Cycle_t currentTime) { return (userLock_ > 0) ? true : ((LLSC_ && (currentTime < LLSCTime_)) ? true : false)  ; }
        uint64_t getLLSCTime() { return LLSC_ ?  LLSCTime_ : 0; }
        void incLock() { userLock_++; }
        void decLock() { userLock_--; }


        // Waiting events
        bool getEventsWaitingForLock() { return eventsWaitingForLock_; }
        void setEventsWaitingForLock(bool eventsWaiting) { eventsWaitingForLock_ = eventsWaiting; }

        ReplacementInfo * getReplacementInfo() override { return info_; }

        // String-ify for debugging
        std::string getString() {
            std::ostringstream str;
            str << "LLSC: " << (LLSC_ ? "Y" : "N");
            str << " LLSCTime: " << LLSCTime_;
            str << " LLSCTid: [";
            for (std::set<uint32_t>::iterator it = LLSCTidBuf_.begin(); it != LLSCTidBuf_.end(); it++) {
                if (it != LLSCTidBuf_.begin()) str << ",";
                str << *it;
            }
            str << "] Lock: (" << userLock_ << "," << eventsWaitingForLock_ << ")";
            return str.str();
        }

        L1CacheLine() = default;
        void serialize_order(SST::Core::Serialization::serializer& ser) override {
            CacheLine::serialize_order(ser);
            SST_SER(LLSC_);
            SST_SER(LLSCTime_);
            SST_SER(LLSCTidBuf_);
            SST_SER(userLock_);
            SST_SER(eventsWaitingForLock_);
            SST_SER(info_);
      }
      ImplementSerializable(SST::MemHierarchy::L1CacheLine);
};

/* With owner/sharer state for shared caches */
class SharedCacheLine : public CacheLine {
    private:
        std::set<std::string> sharers_;
        std::string owner_;
        CoherenceReplacementInfo * info_;
    protected:
        virtual void updateReplacement() override { info_->setState(state_); }
    public:
        SharedCacheLine(uint32_t size, unsigned int index) : CacheLine(size, index), owner_("") {
            info_ = new CoherenceReplacementInfo(index, I, false, false);
        }

        virtual ~SharedCacheLine() {
             delete info_;
        }

        void reset() {
            CacheLine::reset();
            sharers_.clear();
            owner_ = "";
            info_->reset();
        }

        // Sharers
        set<std::string>* getSharers() { return &sharers_; }
        bool isSharer(std::string name) {   return sharers_.find(name) != sharers_.end(); }
        size_t numSharers() { return sharers_.size(); }
        bool hasSharers() { return !sharers_.empty(); }
        bool hasOtherSharers(std::string shr) { return !(sharers_.empty() || (sharers_.size() == 1 && sharers_.find(shr) != sharers_.end())); }
        void addSharer(std::string s) {
            sharers_.insert(s);
            info_->setShared(true);
        }
        void removeSharer(std::string s) {
            sharers_.erase(s);
            info_->setShared(!sharers_.empty());
        }

        // Owner
        std::string getOwner() { return owner_; }
        bool hasOwner() { return !owner_.empty(); }
        void setOwner(std::string owner) {
            owner_ = owner;
            info_->setOwned(true);
        }
        void removeOwner() {
            owner_.clear();
            info_->setOwned(false);
        }

        // Replacement
        ReplacementInfo * getReplacementInfo() override { return info_; }

        // String-ify for debugging
        std::string getString() {
            std::ostringstream str;
            str << "O: " << (owner_.empty() ? "-" : owner_);
            str << " S: [";
            for (std::set<std::string>::iterator it = sharers_.begin(); it != sharers_.end(); it++) {
                if (it != sharers_.begin()) str << ",";
                str << *it;
            }
            str << "]";
            return str.str();
        }

        SharedCacheLine() = default;
        void serialize_order(SST::Core::Serialization::serializer& ser) override {
            CacheLine::serialize_order(ser);
            SST_SER(sharers_);
            SST_SER(owner_);
            SST_SER(info_);
        }
        ImplementSerializable(SST::MemHierarchy::SharedCacheLine);

};

/* With flags indicating whether block is cached abov efor private caches */
class PrivateCacheLine : public CacheLine {
    private:
        bool shared_;
        bool owned_;
        CoherenceReplacementInfo * info_;
    protected:
        virtual void updateReplacement() override { info_->setState(state_); }
    public:
        PrivateCacheLine(uint32_t size, unsigned int index) : CacheLine(size, index), shared_(false), owned_(false) {
            info_ = new CoherenceReplacementInfo(index, I, false, false);
        }

        virtual ~PrivateCacheLine() { }

        void reset() {
            CacheLine::reset();
            shared_ = false;
            owned_ = false;
            info_->reset();
        }

        // Shared
        bool getShared() { return shared_; }
        void setShared(bool s) { shared_ = s; info_->setShared(s);}

        // Owned
        bool getOwned() { return owned_; }
        void setOwned(bool o) { owned_ = o; info_->setOwned(o); }

        // Replacement
        ReplacementInfo * getReplacementInfo() override { return info_; }

        // String-ify for debugging
        std::string getString() {
            std::string str = "O: ";
            str += owned_ ? "Yes" : "No";
            str += " S: ";
            str += shared_ ? "Yes" : "No";
            return str;
        }

        PrivateCacheLine() = default;
        void serialize_order(SST::Core::Serialization::serializer& ser) override {
            CacheLine::serialize_order(ser);
            SST_SER(shared_);
            SST_SER(owned_);
            SST_SER(info_);
        }
        ImplementSerializable(SST::MemHierarchy::PrivateCacheLine);
};

}} // End namespace
#endif // MEMHIERARCHY_LINETYPES_H
