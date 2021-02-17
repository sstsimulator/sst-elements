// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
 * - getAddr() for identifiying a line
 * - getReplacementInfo() for returning the information that a replacement policy might need
 */



/* Line with only holds coherence state - no data */
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
        DirectoryLine(uint32_t size, unsigned int index) : index_(index), addr_(0), state_(I), lastSendTimestamp_(0), wasPrefetch_(false) {
            info_ = new CoherenceReplacementInfo(index, I, false, false);
        }
        virtual ~DirectoryLine() { }

        void reset() {
            state_ = I;
            sharers_.clear();
            owner_ = "";
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
        DataLine(uint8_t size, unsigned int index) : index_(index), addr_(0), tag_(nullptr) {
            data_.resize(size);
            info_ = new CoherenceReplacementInfo(index, I, false, false);
        }
        virtual ~DataLine() { }

        void reset() {
            tag_ = nullptr;
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
        ReplacementInfo* getReplacementInfo() { return tag_ ? tag_->getReplacementInfo() : info_; }

        // String-ify for debugging
        std::string getString() {
            return (tag_ ? "Valid" : "Invalid");
        }
};

/* Base class for a line that contains both data & coherence state */
class CacheLine {
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
        CacheLine(uint32_t size, unsigned int index) : index_(index), addr_(0), state_(I), lastSendTimestamp_(0), wasPrefetch_(false) {
            data_.resize(size);
        }
        virtual ~CacheLine() { }

        void reset() {
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

        // String-ify for debugging
        std::string getString() {
            std::ostringstream str;
            str << std::hex << "0x" << addr_;
            str << " State: " << StateString[state_];
            return str.str();
        }
};

/* With atomic/lock flags for L1 caches */
class L1CacheLine : public CacheLine {
    private:
        unsigned int userLock_;
        bool LLSCAtomic_;
        bool eventsWaitingForLock_;
        ReplacementInfo * info;
    protected:
        void updateReplacement() { info->setState(state_); }
    public:
        L1CacheLine(uint32_t size, unsigned int index) : userLock_(0), LLSCAtomic_(false), eventsWaitingForLock_(false), CacheLine(size, index) {
            info = new ReplacementInfo(index, I);
        }
        virtual ~L1CacheLine() { }

        void reset() {
            CacheLine::reset();
            userLock_ = 0;
            LLSCAtomic_ = false;
            eventsWaitingForLock_ = false;
        }

        // LLSC
        void atomicStart() { LLSCAtomic_ = true; }
        void atomicEnd() { LLSCAtomic_ = false; }
        bool isAtomic() { return LLSCAtomic_; }

        // Lock
        bool isLocked() { return (userLock_ > 0) ? true : false; }
        void incLock() { userLock_++; }
        void decLock() { userLock_--; }

        // Waiting events
        bool getEventsWaitingForLock() { return eventsWaitingForLock_; }
        void setEventsWaitingForLock(bool eventsWaiting) { eventsWaitingForLock_ = eventsWaiting; }

        ReplacementInfo * getReplacementInfo() { return info; }

        // String-ify for debugging
        std::string getString() {
            std::ostringstream str;
            str << "LLSC: " << (LLSCAtomic_ ? "Y" : "N");
            str << " Lock: (" << userLock_ << "," << eventsWaitingForLock_ << ")";
            return str.str();
        }
};

/* With owner/sharer state for shared caches */
class SharedCacheLine : public CacheLine {
    private:
        std::set<std::string> sharers_;
        std::string owner_;
        CoherenceReplacementInfo * info;
    protected:
        virtual void updateReplacement() { info->setState(state_); }
    public:
        SharedCacheLine(uint32_t size, unsigned int index) : owner_(""), CacheLine(size, index) {
            info = new CoherenceReplacementInfo(index, I, false, false);
        }

        virtual ~SharedCacheLine() { }

        void reset() {
            CacheLine::reset();
            sharers_.clear();
            owner_ = "";
        }

        // Sharers
        set<std::string>* getSharers() { return &sharers_; }
        bool isSharer(std::string name) {   return sharers_.find(name) != sharers_.end(); }
        size_t numSharers() { return sharers_.size(); }
        bool hasSharers() { return !sharers_.empty(); }
        bool hasOtherSharers(std::string shr) { return !(sharers_.empty() || (sharers_.size() == 1 && sharers_.find(shr) != sharers_.end())); }
        void addSharer(std::string s) {
            sharers_.insert(s);
            info->setShared(true);
        }
        void removeSharer(std::string s) {
            sharers_.erase(s);
            info->setShared(!sharers_.empty());
        }

        // Owner
        std::string getOwner() { return owner_; }
        bool hasOwner() { return !owner_.empty(); }
        void setOwner(std::string owner) {
            owner_ = owner;
            info->setOwned(true);
        }
        void removeOwner() {
            owner_.clear();
            info->setOwned(false);
        }

        // Replacement
        ReplacementInfo * getReplacementInfo() { return info; }

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
};

/* With flags indicating whether block is cached abov efor private caches */
class PrivateCacheLine : public CacheLine {
    private:
        bool shared;
        bool owned;
        CoherenceReplacementInfo * info;
    protected:
        virtual void updateReplacement() { info->setState(state_); }
    public:
        PrivateCacheLine(uint32_t size, unsigned int index) : shared(false), owned(false), CacheLine(size, index) {
            info = new CoherenceReplacementInfo(index, I, false, false);
        }

        virtual ~PrivateCacheLine() { }

        void reset() {
            CacheLine::reset();
            shared = false;
            owned = false;
        }

        // Shared
        bool getShared() { return shared; }
        void setShared(bool s) { shared = s; info->setShared(s);}

        // Owned
        bool getOwned() { return owned; }
        void setOwned(bool o) { owned = o; info->setOwned(o); }

        // Replacement
        ReplacementInfo * getReplacementInfo() { return info; }

        // String-ify for debugging
        std::string getString() {
            std::string str = "O: ";
            str += owned ? "Yes" : "No";
            str += " S: ";
            str += shared ? "Yes" : "No";
            return str;
        }
};

}} // End namespace
#endif // MEMHIERARCHY_LINETYPES_H
