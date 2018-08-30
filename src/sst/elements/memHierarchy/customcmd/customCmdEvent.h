// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_CUSTOMMEMEVENT_H_
#define _MEMHIERARCHY_CUSTOMMEMEVENT_H_

#include <sst/core/sst_types.h>

#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/memTypes.h"

namespace SST { namespace MemHierarchy {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif

using namespace std;

class CustomCmdEvent : public MemEventBase {
public:

    CustomCmdEvent(std::string src, Addr addr, Addr baseAddr, Command cmd, uint32_t opc = 0, uint32_t size = 0) : 
        MemEventBase(src, cmd), addr_(addr), baseAddr_(baseAddr), addrGlobal_(true), opCode_(opc), size_(size), instPtr_(0), vAddr_(0) { }

    /* Getters/setters */
    void setAddr(Addr addr) { addr_ = addr; }
    Addr getAddr() { return addr_; }

    void setBaseAddr(Addr baseAddr) { baseAddr_ = baseAddr; }
    Addr getBaseAddr() { return baseAddr_; }

    void setAddrGlobal(bool val) { addrGlobal_ = val; }
    bool isAddrGlobal() { return addrGlobal_; }

    void setOpCode(uint32_t opc) { opCode_ = opc; }
    uint32_t getOpCode() { return opCode_; }

    void setSize(uint32_t size) { size_ = size; }
    uint32_t getSize() { return size_; }

    void setPayload(std::vector<uint8_t>& data) {
        setSize(data.size());
        payload_ = data;
    }
    std::vector<uint8_t>& getPayload() { 
        if (payload_.size() < size_) payload_.resize(size_); 
        return payload_;
    }

    void setInstructionPointer(Addr ip) { instPtr_ = ip; }
    Addr getInstructionPointer() { return instPtr_; }

    void setVirtualAddress(Addr va) { vAddr_ = va; }
    Addr getVirtualAddress() { return vAddr_; }

    /* Virtual functions inherited from MemEventBase */
    virtual CustomCmdEvent* makeResponse() override {
        CustomCmdEvent* me = new CustomCmdEvent(*this);
        me->setResponse(this);
        me->instPtr_ = instPtr_;
        me->vAddr_ = vAddr_;
        me->opCode_ = opCode_;
        return me;
    }

    /*** Virtual functions from memEventBase ***/
    virtual uint32_t getEventSize() override { return 0; }

    virtual std::string getVerboseString() override {
        std::ostringstream str;
        str << std::hex << " Addr: 0x" << addr_;
        str << std::hex << " BaseAddr: 0x" << baseAddr_;
        str << (addrGlobal_ ? "(Global)" : "(Local)");
        str << " Size: " << std::dec << size_;
        str << " VA: 0x" << std::hex << vAddr_ << " IP: 0x" << instPtr_;
        str << " OpCode: 0x" << opCode_;
        return MemEventBase::getVerboseString() + str.str();
    }

    virtual std::string getBriefString() {
        std::ostringstream str;
        str << std::hex << " Addr: 0x" << addr_;
        str << std::hex << " BaseAddr: 0x" << baseAddr_;
        str << std::dec << " Size: " << size_;
        str << std::hex << " OpCode: 0x" << opCode_;
        return MemEventBase::getBriefString() + str.str();
    }

    virtual bool doDebug(std::set<Addr> &addr) {
        return (addr.find(baseAddr_) != addr.end());
    }

    virtual Addr getRoutingAddress() {
        return baseAddr_;
    }

    virtual CustomCmdEvent* clone(void) override {
        return new CustomCmdEvent(*this);
    }

    virtual size_t getPayloadSize() {
        return payload_.size();
    }

private:
    Addr                    addr_;
    Addr                    baseAddr_;
    bool                    addrGlobal_;/* Is address global or local? */
    uint32_t                opCode_;    /* Custom Op Code */
    uint32_t                size_;      /* Number of bytes requested, payload size, etc. */
    std::vector<uint8_t>    payload_;   /* Payload, for responses */
    Addr                    instPtr_;   /* Instruction pointer */
    Addr                    vAddr_;     /* Virtual address */

    CustomCmdEvent() : MemEventBase() { } // For serialization only

/* Serialization */
public:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        MemEventBase::serialize_order(ser);
        ser & addr_;
        ser & baseAddr_;
        ser & addrGlobal_;
        ser & opCode_;
        ser & size_;
        ser & payload_;
        ser & instPtr_;
        ser & vAddr_;
    }

    ImplementSerializable(SST::MemHierarchy::CustomCmdEvent);

};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

}}

#endif
