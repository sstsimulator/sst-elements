// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MIRANDA_CUSTOMCMD_OPCODE_H_
#define _MIRANDA_CUSTOMCMD_OPCODE_H_

#include <interfaces/stdMem.h>

namespace SST {
namespace Miranda {

class OpCodeStdMem : public SST::Interfaces::StandardMem::CustomData {
public:
    typedef uint64_t Addr;

    OpCodeStdMem(Addr addr, uint64_t bytes, uint32_t opcode, bool responseNeeded = true) : 
        CustomData(), addr_(addr), bytes_(bytes), opcode_(opcode), response_(responseNeeded) { }
    
    virtual ~OpCodeStdMem() { }
    
    virtual Addr getRoutingAddress() override { return addr_; }
    
    virtual uint64_t getSize() override { return 8; } /* Return 8B, ~ size of a coherence message */

    virtual CustomData* makeResponse() override { return new OpCodeStdMem(addr_, bytes_, opcode_, false); }
    
    virtual bool needsResponse() override { return true; }
    
    /* String-ify for debug */
    virtual std::string getString() override {
        std::ostringstream str;
        str << std::hex << " Addr: 0x" << addr_;
        str << std::dec << " Bytes: 0x" << bytes_;
        str << std::hex << " OpCode: 0x" << opcode_;
        return str.str();
    }
    
    /* Address getter/setter */
    void setAddr(Addr addr) { addr_ = addr; }
    Addr getAddr() { return addr_; }

    /* Bytes (Size of request) setter */
    void setBytes(uint64_t bytes) { bytes_ = bytes; }
    uint64_t getBytes() { return bytes_; }

    /* Op code getter/setter */
    void setOpCode(uint32_t opc) { opcode_ = opc; }
    uint32_t getOpCode() { return opcode_; }

    /* Serialization **/
    // Must be serializable so that CustomMemEvent can be serialized
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        ser & addr_;
        ser & bytes_;
        ser & opcode_;
        ser & response_;
    }
    ImplementSerializable(SST::Miranda::OpCodeStdMem);
    
    
protected:
    OpCodeStdMem() { } /* For serialization only */
    
    /* Members */
    Addr addr_;
    uint64_t bytes_;
    uint32_t opcode_;
    bool response_;
};


} /* namespace Miranda */
} /* namespace SST */
#endif
