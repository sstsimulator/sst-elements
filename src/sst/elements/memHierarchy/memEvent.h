// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARHCY_MEMEVENT_H
#define MEMHIERARHCY_MEMEVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/warnmacros.h>

#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/memTypes.h"

namespace SST { namespace MemHierarchy {

using namespace std;

/**
 * Interface Event used to represent Memory-based communication.
 *
 * This class primarily consists of a Command to perform at a particular address,
 * potentially including data.
 *
 * The command list includes the needed commands to execute cache coherence protocols
 * as well as standard reads and writes to memory.
 */
class MemEvent : public MemEventBase  {
public:
    
    /* Constructor - Coherence control */
    MemEvent(std::string src, Addr addr, Addr baseAddr, Command cmd) : MemEventBase(src, cmd) {
        initialize();
        addr_ = addr;
        baseAddr_ = baseAddr;
    }
    /* Constructor - Events that request data */
    MemEvent(std::string src, Addr addr, Addr baseAddr, Command cmd, uint32_t size) : MemEventBase(src, cmd) {
        initialize();
        addr_ = addr;
        baseAddr_ = baseAddr;
        size_ = size;
    }
    /* Constructor - Events that carry data */
    MemEvent(std::string src, Addr addr, Addr baseAddr, Command cmd, std::vector<uint8_t>& data) : MemEventBase(src, cmd) {
        initialize();
        addr_ = addr;
        baseAddr_ = baseAddr;
        setPayload(data);
    }
    /* Constructor - Events that are not routed by address */
    MemEvent(std::string src, Command cmd) : MemEventBase(src, cmd) {
        initialize();
        addr_ = 0;
        baseAddr_ = 0;
    }

    /** Create a new MemEvent instance, pre-configured to act as a NACK response */
    MemEvent* makeNACKResponse(MemEvent* NACKedEvent) {
        MemEvent *me      = new MemEvent(*this);
        me->setResponse(this);
        me->NACKedEvent_    = NACKedEvent;
        me->cmd_            = Command::NACK;
        me->instPtr_        = instPtr_;
        me->vAddr_          = vAddr_;
        return me;
    }

    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse() override {
        MemEvent *me      = new MemEvent(*this);
        me->setResponse(this);
        me->prefetch_     = prefetch_;
        me->instPtr_      = instPtr_;
        me->vAddr_        = vAddr_;
        return me;
    }

    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse(State UNUSED(state)) {
        MemEvent *me = makeResponse();
        return me;
    }

    /** Generate a new MemEvent, pre-populated as a response
     * with a non-default response cmd
     */
    MemEvent* makeResponse(Command cmd) {
        MemEvent *me = makeResponse();
        me->setCmd(cmd);
        return me;
    }

    void copyMetadata(MemEventBase* ev) override {
        MemEventBase::copyMetadata(ev);
        MemEvent * ev1 = static_cast<MemEvent*>(ev);
        vAddr_ = ev1->getVirtualAddress();
        instPtr_ = ev1->getInstructionPointer();
    }

    void initialize() {
        addr_               = 0;
        baseAddr_           = 0;
        addrGlobal_         = true;
        size_               = 0;
        prefetch_           = false;
        NACKedEvent_        = nullptr;
        retries_            = 0;
        payload_.clear();
        dirty_              = false;
	instPtr_	    = 0;
	vAddr_		    = 0;
        isEvict_            = false;
    }

    /** return the original event that caused a NACK */
    MemEvent* getNACKedEvent() { return NACKedEvent_; }

    /** @return  the target Address of this MemEvent */
    Addr getAddr(void) const { return addr_; }
    /** Sets the target Address of this MemEvent */
    void setAddr(Addr addr) { addr_ = addr; }

    /** Sets the Base Address of this MemEvent */
    void setBaseAddr(Addr baseAddr) { baseAddr_ = baseAddr; }
    /** Return the BaseAddr */
    Addr getBaseAddr() { return baseAddr_; }

    /** Sets whether the address is global (T) or local (F) */
    void setAddrGlobal(bool global) { addrGlobal_ = global; }
    /** Return whether address is global (T) or local (F) */
    bool isAddrGlobal() { return addrGlobal_; }

    /** Sets the virtual address of this MemEvent */
    void setVirtualAddress(Addr newVA) { vAddr_ = newVA; }
    /** Gets the virtual address of this MemEvent */
    uint64_t getVirtualAddress() const { return vAddr_; }

    /** Sets the instruction pointer of that caused this MemEvent */
    void setInstructionPointer(Addr newIP) { instPtr_ = newIP; }
    /** Get the instruction pointer of that caused this MemEvent */
    uint64_t getInstructionPointer() const { return instPtr_; }

    /** @return  the size in bytes that this MemEvent represents */
    uint32_t getSize(void) const { return size_; }
    /** Sets the size in bytes that this MemEvent represents */
    void setSize(uint32_t size) { size_ = size; }

    /** Increments the number of retries */
    void incrementRetries() { retries_++; }
    int getRetries() { return retries_; }
    
    void setReadLock() { setFlag(MemEventBase::F_LOCKED); }
    bool isReadLock() { return cmd_ == Command::GetSX && queryFlag(MemEventBase::F_LOCKED); }

    void setLoadLink() { setFlag(MemEventBase::F_LLSC); }
    bool isLoadLink() { return cmd_ == Command::GetSX && queryFlag(MemEventBase::F_LLSC); }

    void setStoreConditional() { setFlag(MemEventBase::F_LLSC); }
    bool isStoreConditional() { return (cmd_ == Command::GetX || cmd_ == Command::Write) && queryFlag(MemEventBase::F_LLSC); }

    void setFail() { setFlag(MemEventBase::F_FAIL); }
    void setSuccess(bool b) { b ? clearFlag(MemEventBase::F_FAIL) : setFlag(MemEventBase::F_FAIL); }
    bool success() { return !queryFlag(MemEventBase::F_FAIL); }

    /** @return  the data payload. */
    dataVec& getPayload(void) {
        /* Lazily allocate space for payload */
        if ( payload_.size() < size_ )  payload_.resize(size_);
        return payload_;
    }


    /** Sets the data payload and payload size.
     * @param[in] data  Vector from which to copy data
     */
    void setPayload(std::vector<uint8_t>& data) {
        setSize(data.size());
        payload_ = data;
    }

    /** Sets the data payload and payload size.
     * @param[in] size  How many bytes to copy from data
     * @param[in] data  Data array to set as payload
     */
    void setPayload(uint32_t size, uint8_t* data) {
        setSize(size);
        payload_.resize(size);
        for ( uint32_t i = 0 ; i < size ; i++ ) {
            payload_[i] = data[i];
        }
    }

    void setZeroPayload(uint32_t size) {
        setSize(size);
        payload_.clear();
        payload_.resize(size, 0);
    }

    size_t getPayloadSize() override {
        return payload_.size();
    }

    /** Sets that this is a prefetch command */
    void setPrefetchFlag(bool prefetch) { prefetch_ = prefetch;}
    /** Returns true if this is a prefetch command */
    bool isPrefetch() { return prefetch_; }

// Information about command types
    /** Returns true if this is a request that needs to access the data array (Get/Put/Flush) */
    bool isDataRequest(void) const { return CommandClassArr[(int)cmd_] == CommandClass::Request; }
    /** Returns true if this is of response type */
    bool isResponse(void) const { return BasicCommandClassArr[(int)cmd_] == BasicCommandClass::Response; }
    /** Returns true if this is a writeback */
    bool isWriteback(void) const { return CommandWriteback[(int)cmd_]; }
    /** Returns true if this request type should be routed by address (versus destination) */
    bool isRoutedByAddress(void) const { return CommandRouteByAddress[(int)cmd_]; }

    void setEvict(bool status) { isEvict_ = status; }
    bool getEvict() { return isEvict_; }

    void setDirty(bool status) { dirty_ = status; }
    bool getDirty() { return dirty_; }

    virtual MemEvent* clone(void) override {
        return new MemEvent(*this);
    }

    virtual std::string getVerboseString(int level = 1) override {
        std::ostringstream str;
        if (addr_ != baseAddr_)
            str << std::hex << " Addr: 0x" << baseAddr_ << "/0x" << addr_;
        else
            str << std::hex << " Addr: 0x" << baseAddr_;
        str << (addrGlobal_ ? " (G)" : " (L)");
        if (payload_.empty() || level < 11)
            str << " Data: " << (payload_.empty() ? "F" : "T");
        else {
            std::stringstream value;
            value << std::hex << std::setfill('0');
            for (unsigned int i = 0; i < payload_.size(); i++)
                value << std::hex << std::setw(2) << (int)payload_[i];
            str << " Data: 0x" << value.str();
        }
        str << " VA: 0x" << vAddr_ << " IP: 0x" << instPtr_;
        str << std::dec << " Size: " << size_;
        str << " Prf: " << (prefetch_ ? "T" : "F");
        return MemEventBase::getVerboseString(level) + str.str();
    }

    virtual std::string toString() const override {
        std::ostringstream str;
        if (addr_ != baseAddr_)
            str << std::hex << " Addr: 0x" << baseAddr_ << "/0x" << addr_;
        else
            str << std::hex << " Addr: 0x" << baseAddr_;
        str << std::dec << " Size: " << size_;
        return MemEventBase::toString() + str.str();
    }

    virtual std::string getBriefString() override {
        std::ostringstream str;
        if (addr_ != baseAddr_)
            str << std::hex << " Addr: 0x" << baseAddr_ << "/0x" << addr_;
        else
            str << std::hex << " Addr: 0x" << baseAddr_;
        str << std::dec << " Size: " << size_;
        return MemEventBase::getBriefString() + str.str();
    }

    virtual bool doDebug(std::set<Addr> &addr) override {
        if (cmd_ == Command::NULLCMD && addr.find(addr_) != addr.end())
            return true;
        return (addr.find(baseAddr_) != addr.end());
    }

    virtual Addr getRoutingAddress() override {
        return baseAddr_;
    }

private:
    uint32_t        size_;              // Size in bytes that are being requested
    Addr            addr_;              // Address
    Addr            baseAddr_;          // Base (line) address
    bool            addrGlobal_;        // Whether address is a local or global address
    MemEvent*       NACKedEvent_;       // For a NACK, pointer to the NACKed event
    int             retries_;           // For NACKed events, how many times a retry has been sent
    dataVec         payload_;           // Data
    bool            prefetch_;          // Whether this request came from a prefetcher
    bool            dirty_;             // For a replacement, whether the data is dirty or not
    bool            isEvict_;           // Whether an event is an eviction
    Addr	    instPtr_;           // Instruction pointer associated with the request
    Addr 	    vAddr_;             // Virtual address associated with the request

    MemEvent() : MemEventBase() {} // For serialization only

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        MemEventBase::serialize_order(ser);
        ser & size_;
        ser & addr_;
        ser & baseAddr_;
        ser & addrGlobal_;
        ser & NACKedEvent_;
        ser & retries_;
        ser & payload_;
        ser & prefetch_;
        ser & dirty_;
        ser & isEvict_;
        ser & instPtr_;
        ser & vAddr_;
    }

    ImplementSerializable(SST::MemHierarchy::MemEvent);
};

}}

#endif /* INTERFACES_MEMEVENT_H */
