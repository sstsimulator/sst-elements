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

#ifndef MEMHIERARHCY_MEMEVENTBASE_H
#define MEMHIERARHCY_MEMEVENTBASE_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/event.h>
#include "sst/core/element.h"

#include "util.h"
#include "memTypes.h"

namespace SST { namespace MemHierarchy {

using namespace std;

/**
 * Base class for memH events
 *
 */
class MemEventBase : public SST::Event  {
public:
 
    typedef std::vector<uint8_t> dataVec;       /** Data Payload type */

    // Flags used throughout memHierarchy
    static const uint32_t F_LOCKED          = 0x00000001;
    static const uint32_t F_NONCACHEABLE    = 0x00000010;
    static const uint32_t F_LLSC            = 0x00000100;
    static const uint32_t F_SUCCESS         = 0x00001000;
    static const uint32_t F_NORESPONSE      = 0x00010000;


    /** Creates a new MemEventBase */
    MemEventBase(std::string src, Command cmd) : SST::Event() {
        setDefaults();
        cmd_ = cmd;
        src_ = src;
    }

    virtual void setDefaults() {
        eventID_        = generateUniqueId();  // Defined in SST::Event
        responseToID_   = NO_ID;
        dst_            = NONE;
        src_            = NONE;
        rqstr_          = NONE;
        cmd_            = Command::NULLCMD;
        flags_          = 0;
        memFlags_       = 0;
    }

    void setResponse(MemEventBase * event) {
        responseToID_ = event->eventID_;
        cmd_ = CommandResponse[(int)cmd_];
        dst_ = event->src_;
        src_ = event->dst_;
        rqstr_ = event->rqstr_;
        flags_ = event->flags_;
        memFlags_ = event->memFlags_;
    }

    /** @return  Unique ID of this MemEvent */
    id_type getID(void) const { return eventID_; }
    
    /** @return  Unique ID of the MemEvent that this is a response to */
    id_type getResponseToID(void) const { return responseToID_; }
    
    /** @return  Command of this MemEvent */
    Command getCmd(void) const { return cmd_; }
    /** Sets the Command of this MemEvent */
    void setCmd(Command newcmd) { cmd_ = newcmd; }
    
    /** @return the source string - who sent this MemEvent */
    const std::string& getSrc(void) const { return src_; }
    /** Sets the source string - who sent this MemEvent */
    void setSrc(const std::string& src) { src_ = src; }
    
    /** @return the destination string - who receives this MemEvent */
    const std::string& getDst(void) const { return dst_; }
    /** Sets the destination string - who received this MemEvent */
    void setDst(const std::string& dst) { dst_ = dst; }
    
    /** @return the requestor string - whose original request caused this MemEvent */
    const std::string& getRqstr(void) const { return rqstr_; }
    /** Sets the requestor string - whose original request caused this MemEvent */
    void setRqstr(const std::string& rqstr) { rqstr_ = rqstr; }

    /** @returns the state of all flags */
    uint32_t getFlags(void) const { return flags_; }
    /** Sets the specified flag.
     * @param[in] flag  Should be one of the flags 'F_...' defined in MemEventBase */
    void setFlag(uint32_t flag) { flags_ = flags_ | flag; }
    /** Clears the specified flag.
     * @param[in] flag  
     */
    void clearFlag(uint32_t flag) { flags_ = flags_ & (~flag); }
    /** Clears all flags */
    void clearFlag(void) { flags_ = 0; }
    /** Check to see if a flag is set
     * @param[in] flag  One of the 'F_..' flags defined in MemEventBase
     * @returns TRUE if flag is set, else FALSE
     */
    bool queryFlag(uint32_t flag) const { return flags_ & flag; };
    /** Sets the entire flag set */
    void setFlags(uint32_t flags) { flags_ = flags; }

    void setMemFlags(uint32_t flags) { memFlags_ = flags; }
    uint32_t getMemFlags() const { return memFlags_; }


    /** Return size of the event - for calculating bandwidth used */
    virtual uint32_t getEventSize() { return 0; }
    
    /** Get verbose print of the event */
    virtual std::string getVerboseString() {
        std::string cmdStr(CommandString[(int)cmd_]);
        return "Cmd: " + cmdStr + " Src: " + src_ + " Dst: " + dst_ + " Rqstr: " + rqstr_;
    }

    /** Get brief print of the event */
    virtual std::string getBriefString() {
        std::string cmdStr(CommandString[(int)cmd_]);
        return "Cmd: " + cmdStr + " Src: " + src_;
    }

    virtual MemEventBase* clone(void) override {
        return new MemEventBase(*this);
    }

protected:
    id_type         eventID_;           // Unique ID for this event
    id_type         responseToID_;      // For responses, holds the ID to which this event matches
    string          src_;               // Source ID
    string          dst_;               // Destination ID
    string          rqstr_;             // Cache that originated this request
    Command         cmd_;               // Command
    uint32_t        flags_;
    uint32_t        memFlags_;

    MemEventBase() {} // For serialization only

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & eventID_;
        ser & responseToID_;
        ser & src_;
        ser & dst_;
        ser & rqstr_;
        ser & cmd_;
        ser & flags_;
        ser & memFlags_;
    }
     
    ImplementSerializable(SST::MemHierarchy::MemEventBase);     
};


/* 
 * Init event type
 */

class MemEventInit : public MemEventBase  {
public:
    MemEventInit(std::string src, Command cmd, Endpoint type, bool inclusive) : MemEventBase(src, cmd), type_(type), inclusive_(inclusive) { }
    MemEventInit(std::string src, Command cmd, Addr addr, std::vector<uint8_t> &data) : MemEventBase(src, cmd), addr_(addr), payload_(data) { }

    Endpoint getType() { return type_; }
    bool getInclusive() { return inclusive_; }
    std::vector<uint8_t>& getPayload() { return payload_; }
    Addr getAddr() { return addr_; }
    void setAddr(Addr addr) { addr_ = addr; }

    virtual MemEventInit* clone(void) override {
        return new MemEventInit(*this);
    }

private:
    // For determining endpoint information 
    Endpoint type_;  // Type of this endpoint
    bool inclusive_; // Whether this endpoint is inclusive
    
    // For pre-loading data into memory
    Addr addr_;
    std::vector<uint8_t> payload_;

    MemEventInit() {} // For serialization only
public:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        MemEventBase::serialize_order(ser);
        ser & type_;
        ser & inclusive_;
        ser & addr_;
        ser & payload_;
    }

    ImplementSerializable(SST::MemHierarchy::MemEventInit);
};

}}


#endif /* INTERFACES_MEMEVENT_H */
