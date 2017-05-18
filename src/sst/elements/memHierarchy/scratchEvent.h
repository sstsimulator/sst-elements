// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARHCY_SCRATCHEVENT_H
#define MEMHIERARHCY_SCRATCHEVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include "sst/core/element.h"
#include "util.h"

namespace SST { namespace MemHierarchy {

using namespace std;
typedef uint64_t Addr;

// List of commands
// Command, ResponseCmd, BasicCommandClass, CommandClass

#define CMD_TYPES \
    CMD(ScratchNullCmd, ScratchNullCmd, Request,    Request) \
    CMD(Read,           ReadResp,       Request,    Request) \
    CMD(Write,          WriteResp,      Request,    Request) \
    CMD(ReadResp,       ScratchNullCmd, Response,   Data) \
    CMD(WriteResp,      ScratchNullCmd, Response,   Ack) \
    CMD(ScratchPut,     WriteResp,      Request,    Request) \
    CMD(ScratchGet,     ReadResp,       Request,    Request)

// Create ScratchCommand enum
enum ScratchCommand {
#define CMD(a,b,c,d) a,
    CMD_TYPES
#undef CMD
    NUM_STATES
};

static const ScratchCommand ScratchCommandResponse[] = {
#define CMD(a,b,c,d) b,
    CMD_TYPES
#undef CMD
};

static const char* ScratchCommandString[] = {
#define CMD(a,b,c,d) #a,
    CMD_TYPES
#undef CMD
};

static const BasicCommandClass ScratchBasicCommandClass[] = {
#define CMD(a,b,c,d) BasicCommandClass::c,
    CMD_TYPES
#undef CMD
};

static const CommandClass ScratchCommandClass[] = {
#define CMD(a,b,c,d) CommandClass::d,
    CMD_TYPES
#undef CMD
};

#undef CMD_TYPES

/**
 * Event used in scratchpad-oriented memory systesm
 *
 *  Field       Type        Def
 *  cmd         Command     Event type
 *  src         string      Who sent this event
 *  dst         string      Who should receive this event
 *  addr        uint64_t    Address for data, for get/put the target address
 *  baseAddr    uint64_t    Line-aligned address, (target address for get/put)
 *  srcAddr     uint64_t    Source address for a Get/Put
 *  srcBaseAddr uint64_t    Source line-aligned address for a Get/Put
 *  data        dataVec     Payload if needed (Write, DataResp, ReadResp)
 *  size        uint32_t    Requested number of bytes (or if a payload is included, size of the payload)
 *  memFlags    uint32_t    Flags that are directed to memory backends
 *  ***** Special fields for statistics, tracking, etc. ******
 *  vAddr       uint64_t    Virtual address of request
 *  iPtr        uint64_t    Instruction pointer of request
 *  rqstr       string      Who initiated the request that led to this event
 *  id          Event ID    Unique identifier for this event
 */
class ScratchEvent : public SST::Event  {
public:

    typedef std::vector<uint8_t> dataVec;       /** Data Payload type */

    /** Creates a new ScratchEvent - Generic */
    ScratchEvent(std::string src, Addr addr, Addr baseAddr, ScratchCommand cmd) : SST::Event() {
        initialize();
        src_ = src;
        addr_ = addr;
        baseAddr_ = baseAddr;
        cmd_ = cmd;
    }

    /** ScratchEvent constructor - no payload */
    ScratchEvent(std::string src, Addr addr, Addr baseAddr, ScratchCommand cmd, uint32_t size) : SST::Event() {
        initialize();
        src_ = src;
        addr_ = addr;
        baseAddr_ = baseAddr;
        cmd_ = cmd;
        size_ = size;
    }

    /** ScratchEvent constructor - with payload */
    ScratchEvent(std::string src, Addr addr, Addr baseAddr, ScratchCommand cmd, std::vector<uint8_t>& data) : SST::Event() {
        initialize();
        src_ = src;
        addr_ = addr;
        baseAddr_ = baseAddr;
        cmd_ = cmd;
        setPayload(data);
    }

    ScratchEvent * makeResponse() {
        ScratchEvent * ev = new ScratchEvent(*this);
        ev->setCmd(ScratchCommandResponse[cmd_]);
        ev->setDst(src_);
        ev->setSrc(dst_);
        ev->setRqstr(rqstr_);
        ev->setVirtualAddress(vAddr_);
        ev->setInstructionPointer(iPtr_);
        ev->setRespID(id_);
        return ev;
    }

    void initialize() {
        cmd_            = ScratchNullCmd;
        src_            = "";
        dst_            = "";
        addr_           = 0;
        baseAddr_       = 0;
        srcAddr_        = 0;
        srcBaseAddr_    = 0;
        data_.clear();
        size_           = 0;
        memFlags_       = 0;

        vAddr_          = 0;
        iPtr_           = 0;
        id_             = generateUniqueId();
        respID_         = NO_ID;
        rqstr_          = "";
    }

// Getters and setters
    
    // cmd
    ScratchCommand getCmd(void) const { return cmd_; }
    void setCmd(ScratchCommand cmd) { cmd_ = cmd; }
    
    // src
    const std::string& getSrc(void) const { return src_; }
    void setSrc(const std::string& src) { src_ = src; }
    
    // dst
    const std::string& getDst(void) const { return dst_; }
    void setDst(const std::string& dst) { dst_ = dst; }
    
    // addr
    Addr getAddr(void) const { return addr_; }
    void setAddr(Addr addr) { addr_ = addr; }
    
    // baseAddr
    Addr getBaseAddr(void) const { return baseAddr_; }
    void setBaseAddr(Addr addr) { baseAddr_ = addr; }
    
    // srcAddr
    Addr getSrcAddr(void) const { return srcAddr_; }
    void setSrcAddr(Addr addr) { srcAddr_ = addr; }
    
    // srcBaseAddr
    Addr getSrcBaseAddr(void) const { return srcBaseAddr_; }
    void setSrcBaseAddr(Addr addr) { srcBaseAddr_ = addr; }
    
    // data
    dataVec& getPayload(void) {
        if ( data_.size() < size_ )  data_.resize(size_);
        return data_;
    }
    void setPayload(std::vector<uint8_t>& data) {
        setSize(data.size());
        data_ = data;
    }
    
    // size
    uint32_t getSize(void) const { return size_; }
    void setSize(uint32_t size) { size_ = size; }
    
    // memFlags
    uint32_t getMemFlags(void) const { return memFlags_; }
    void setMemFlags(uint32_t flag) { memFlags_ = memFlags_ | flag; }
    
    // vAddr
    Addr getVirtualAddress(void) const { return vAddr_; }
    void setVirtualAddress(Addr addr) { vAddr_ = addr; }
    
    // iPtr
    Addr getInstructionPointer(void) const { return iPtr_; }
    void setInstructionPointer(Addr iptr) { iPtr_ = iptr; }
    
    // id
    id_type getID(void) const { return id_; }
    
    // respID
    id_type getRespID(void) const { return respID_; }
    void setRespID(id_type id) { respID_ = id; }

    // rqstr
    const std::string& getRqstr() const { return rqstr_; }
    void setRqstr(const std::string& rqstr) { rqstr_ = rqstr; }

private:
    ScratchCommand  cmd_;           // Event type
    std::string     src_;           // Event source
    std::string     dst_;           // Event destination
    Addr            addr_;          // Address (target address if this is a get/put)
    Addr            baseAddr_;      // Base address (line-aligned address, target if a get/put)
    Addr            srcAddr_;       // Source address if this is a get/put
    Addr            srcBaseAddr_;   // Source base address if this is a get/put
    dataVec         data_;          // Data payload
    uint32_t        size_;          // Either size of data payload or requested # of bytes
    uint32_t        memFlags_;      // Flags for memory backend
    Addr            vAddr_;         // Virtual address of the request
    Addr            iPtr_;          // Instruction pointer of the request
    std::string     rqstr_;         // Requestor who's request initiated this event
    id_type         id_;            // Unique ID for this event
    id_type         respID_;        // Unique ID of request to which this event is the response

    ScratchEvent() {} // For serialization only

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & cmd_;
        ser & src_;
        ser & dst_;
        ser & addr_;
        ser & baseAddr_;
        ser & srcAddr_;
        ser & srcBaseAddr_;
        ser & data_;
        ser & size_;
        ser & memFlags_;
        ser & vAddr_;
        ser & iPtr_;
        ser & rqstr_;
        ser & id_;
        ser & respID_;
    }
     
    ImplementSerializable(SST::MemHierarchy::ScratchEvent);     
};

}}

#endif /* MEMHIERARCHY_SCRATCHEVENT_H */
