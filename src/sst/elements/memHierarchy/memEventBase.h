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

#ifndef MEMHIERARCHY_MEMEVENTBASE_H
#define MEMHIERARCHY_MEMEVENTBASE_H

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/warnmacros.h>

#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memTypes.h"

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
    static const uint32_t F_FAIL            = 0x00001000;
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

    virtual MemEventBase* makeResponse() {
        MemEventBase * me = new MemEventBase(*this);
        me->setResponse(this);
        return me;
    }

    void setResponse(MemEventBase * event) {
        responseToID_ = event->eventID_;
        cmd_ = CommandResponse[(int)cmd_];
        dst_ = event->src_;
        src_ = event->dst_;
        rqstr_ = event->rqstr_;
        tid_ = event->tid_;
        flags_ = event->flags_;
        memFlags_ = event->memFlags_;
    }

    virtual void copyMetadata(MemEventBase* ev) {
        rqstr_ = ev->rqstr_;
        tid_ = ev->tid_;
        flags_ = ev->flags_;
        memFlags_ = ev->memFlags_;
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

    /** @return the thread ID that originated the original request */
    [[deprecated("Use getThreadID() instead (with capital 'D')")]]
    const uint32_t getThreadId(void) const { return tid_; }
    const uint32_t getThreadID(void) const { return tid_; }
    /** Sets the thread ID that originated the original request */
    void setThreadID(const uint32_t tid) { tid_ = tid; }
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

    std::string getFlagString() {
        std::string str;
        str += "[";
        bool addComma = false;
        if (flags_ & F_LOCKED) {
            str += "F_LOCKED";
            addComma = true;
        }
        if (flags_ & F_NONCACHEABLE) {
            if (addComma) str += ", ";
            str += "F_NONCACHEABLE";
            addComma = true;
        }
        if (flags_ & F_LLSC) {
            if (addComma) str += ", ";
            str += "F_LLSC";
            addComma = true;
        }
        if (flags_ & F_FAIL) {
            if (addComma) str += ", ";
            str += "F_FAIL";
            addComma = true;
        }
        if (flags_ & F_NORESPONSE) {
            if (addComma) str += ", ";
            str += "F_NORESPONSE";
            addComma = true;
        }
        str += "]";
        return str;
    }

    /** Return size of the event - for calculating bandwidth used */
    virtual uint32_t getEventSize() { return 0; }

    /** Get verbose print of the event */
    virtual std::string getVerboseString(int level = 1) {
        std::ostringstream idstring;
        idstring << "<" << eventID_.first << "," << eventID_.second << "> ";
        std::string cmdStr(CommandString[(int)cmd_]);
        std::ostringstream str;
        str << " Flags: " << getFlagString();
        return idstring.str() + cmdStr + " Src: " + src_ + " Dst: " + dst_ + " Rq: " + rqstr_ + " Tid: " + std::to_string(tid_) + str.str();
    }

    /** Get brief print of the event */
    virtual std::string toString() const override {
        std::string cmdStr(CommandString[(int)cmd_]);
        std::ostringstream idstring;
        idstring << "<" << eventID_.first << "," << eventID_.second << "> ";
        return idstring.str() + cmdStr + " Src: " + src_ + " Dst: " + dst_ + " Tid: " + std::to_string(tid_);
    }

    /** Get brief print of the event */
    virtual std::string getBriefString() {
        std::string cmdStr(CommandString[(int)cmd_]);
        std::ostringstream idstring;
        idstring << "<" << eventID_.first << "," << eventID_.second << "> ";
        return idstring.str() + cmdStr + " Src: " + src_ + " Dst: " + dst_ + " Tid: " + std::to_string(tid_);
    }

    virtual bool doDebug(std::set<Addr> &UNUSED(addr)) {
        return true;    // Always debug unless we come up with a different way of determining it
    }

    virtual Addr getRoutingAddress() {
        return 0;   // Route this as if its address was 0
    }

    virtual size_t getPayloadSize() {
        return 0; // No payload
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
    uint32_t        tid_;               // Thread ID that originated this request
    Command         cmd_;               // Command
    uint32_t        flags_;
    uint32_t        memFlags_;

    MemEventBase() {} // For serialization only

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        SST_SER(eventID_);
        SST_SER(responseToID_);
        SST_SER(src_);
        SST_SER(dst_);
        SST_SER(rqstr_);
        SST_SER(tid_);
        SST_SER(cmd_);
        SST_SER(flags_);
        SST_SER(memFlags_);
    }

    ImplementSerializable(SST::MemHierarchy::MemEventBase);
};

struct memEventCmp {
    bool operator() (const MemEventBase * lhs, const MemEventBase * rhs) const {
        return lhs->getID() < rhs->getID();
    }
};

/*
 * Untimed event type
 */

class MemEventInit : public MemEventBase  {
public:

    enum class InitCommand { Region, Data, Coherence, Endpoint, Flush };

    /* Untimed event */
    MemEventInit(std::string src, InitCommand cmd) : MemEventBase(src, Command::NULLCMD), initCmd_(cmd) { }

    /* Untimed memory events that carry data */
    MemEventInit(std::string src, Command cmd, Addr addr, std::vector<uint8_t> &data) :
        MemEventBase(src, cmd), initCmd_(InitCommand::Data), addr_(addr), size_(0), payload_(data) { }

    /* Untimed memory events without data */
    MemEventInit(std::string src, Command cmd, Addr addr, size_t size) :
        MemEventBase(src, cmd), initCmd_(InitCommand::Data), addr_(addr), size_(size) { }

    /** Generate a new MemEventInit, pre-populated as a response */
    MemEventInit* makeResponse() override {
        MemEventInit *me      = new MemEventInit(*this);
        me->setResponse(this);
        return me;
    }

    InitCommand getInitCmd() { return initCmd_; }

    Addr getAddr() { return addr_; }
    void setAddr(Addr addr) { addr_ = addr; }

    size_t getSize() { return payload_.empty() ? payload_.size() : size_; }
    std::vector<uint8_t>& getPayload() { return payload_; }
    void setPayload(std::vector<uint8_t> &data) { payload_ = data; }

    virtual MemEventInit* clone(void) override {
        return new MemEventInit(*this);
    }

    virtual std::string getVerboseString(int level = 1) override {
        std::stringstream str;
        if (initCmd_ == InitCommand::Region) str << " InitCmd: Region";
        else if (initCmd_ == InitCommand::Data)
        {
            str << " InitCmd: Data (0x" <<std::hex << addr_ << ", " << std::dec << size_ << ")";
        }
        else if (initCmd_ == InitCommand::Coherence) str << " InitCmd: Coherence";
        else if (initCmd_ == InitCommand::Endpoint) str << " InitCmd: Endpoint";
        else if (initCmd_ == InitCommand::Flush) str << " InitCmd: Flush";
        else str << " InitCmd: Unknown command";

        return MemEventBase::getVerboseString(level) + str.str();
    }

    virtual std::string getBriefString() override {
        std::string str;
        if (initCmd_ == InitCommand::Region) str = " InitCmd: Region";
        else if (initCmd_ == InitCommand::Data) str = " InitCmd: Data";
        else if (initCmd_ == InitCommand::Coherence) str = " InitCmd: Coherence";
        else if (initCmd_ == InitCommand::Endpoint) str = " InitCmd: Endpoint";
        else str = " InitCmd: Unknown command";

        return MemEventBase::getBriefString() + str;
    }

    virtual Addr getRoutingAddress() override { return addr_; }


protected:
    InitCommand initCmd_;

    // For memory events prior to run loop
    // Pre-load data into memory and read it
    // MMIO device init
    Addr addr_;
    size_t size_;
    std::vector<uint8_t> payload_;

    MemEventInit() {} // For serialization only
public:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        MemEventBase::serialize_order(ser);
        SST_SER(initCmd_);
        SST_SER(addr_);
        SST_SER(size_);
        SST_SER(payload_);
    }

    ImplementSerializable(SST::MemHierarchy::MemEventInit);
};

class MemEventInitCoherence : public MemEventInit  {
public:

    /* Init events for coordintating coherence policies */
    /*
     * type: cache, memory, scratchpad, etc.
     * inclusive: inclusive (true) or noninclusive (false)
     * sendWBAck: the component always sends WB Acks (if false, the component *can* send them if another component needs it)
     * recvWBAck: the component expects to receive WB Acks (if false, the component *can* expect to receive them if another component sends them)
     * lineSize: number of bytes in a line
     * tracksPresence: whether the component keeps track of whether a line is present elsewhere. Affects whether clean evictions need to happen or not.
     */
    MemEventInitCoherence(std::string src, Endpoint type, bool inclusive, bool sendWBAck, Addr lineSize, bool tracksPresence) :
        MemEventInit(src, InitCommand::Coherence), type_(type), inclusive_(inclusive), sendWBAck_(sendWBAck), recvWBAck_(false), lineSize_(lineSize), tracksPresence_(tracksPresence) { }
    MemEventInitCoherence(std::string src, Endpoint type, bool inclusive, bool sendWBAck, bool recvWBAck, Addr lineSize, bool tracksPresence) :
        MemEventInit(src, InitCommand::Coherence), type_(type), inclusive_(inclusive), sendWBAck_(sendWBAck), recvWBAck_(recvWBAck), lineSize_(lineSize), tracksPresence_(tracksPresence) { }

    Endpoint getType() { return type_; }
    bool getInclusive() { return inclusive_; }
    bool getSendWBAck() { return sendWBAck_; }
    bool getRecvWBAck() { return recvWBAck_; }
    Addr getLineSize() { return lineSize_; }
    bool getTracksPresence() { return tracksPresence_; }

    virtual MemEventInitCoherence* clone(void) override {
        return new MemEventInitCoherence(*this);
    }

    virtual std::string getVerboseString(int level = 1) override {
        std::ostringstream str;
        str << " Type: " << (int) type_ << " Inclusive: " << (inclusive_ ? "true" : "false");
        str << " LineSize: " << lineSize_ << " Tracks presence: " << (tracksPresence_ ? "true" : "false");
        return MemEventInit::getVerboseString(level) + str.str();
    }

private:
    Endpoint type_;     // Type of endpoint
    bool inclusive_;    // Whether endpoint is inclusive
    bool sendWBAck_;    // Whether endpoint expects writeback acks -> should we send WB acks to the sender?
    bool recvWBAck_;    // Whether endpoint sends writeback acks -> will we receive WB acks from the sender?
    Addr lineSize_;     // Endpoint's linesize
    bool tracksPresence_;     // Endpoint manages or tracks coherence

    MemEventInitCoherence() {} // For serialization only

public:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        MemEventInit::serialize_order(ser);
        SST_SER(type_);
        SST_SER(inclusive_);
        SST_SER(sendWBAck_);
        SST_SER(recvWBAck_);
        SST_SER(lineSize_);
        SST_SER(tracksPresence_);
    }

    ImplementSerializable(SST::MemHierarchy::MemEventInitCoherence);
};

class MemEventInitEndpoint : public MemEventInit {
public:
    /* Init events for coordinating between endpoint interfaces */
    /*
     * type: endpoint type (CPU, MMIO, etc.)
     * name: endpoint name
     * noncacheableRegions: regions that this endpoint is declaring noncacheable
     *
     * TODO: Possibliy merge this with the coherence init messages and broadcast all topology info everywhere
     */

    MemEventInitEndpoint(std::string src, Endpoint type, MemRegion region, bool cacheable) :
        MemEventInit(src, InitCommand::Endpoint), type_(type), name_(src)  {
        regions_.push_back(std::make_pair(region, cacheable));
    }

    Endpoint getType() { return type_; }
    std::string getName() { return name_; }
    std::vector<std::pair<MemRegion,bool>> getRegions() { return regions_; }
    void addRegion(MemRegion reg, bool cacheable) { regions_.push_back(std::make_pair(reg, cacheable)); }

    virtual MemEventInitEndpoint* clone(void) override {
        MemEventInitEndpoint* ep = new MemEventInitEndpoint(*this);
        ep->regions_ = this->regions_;
        return ep;
    }

    virtual std::string getBriefString() override {
        std::ostringstream str;
        str << " Type: " << (int) type_ << " Name: " << name_;
        return MemEventInit::getBriefString() + str.str();
    }

    virtual std::string getVerboseString(int level = 1) override {
        std::ostringstream str;
        str << " Type: " << (int) type_ << " Name: " << name_;
        str << " Regions:";
        for (std::vector<std::pair<MemRegion,bool>>::iterator it = regions_.begin(); it != regions_.end(); it++) {
            str << " [" << it->first.toString() << "; " << (it->second ? "cacheable" : "noncacheable") << "]";
        }
        return MemEventInit::getBriefString() + str.str();
    }

private:
    Endpoint type_;
    std::string name_;
    std::vector<std::pair<MemRegion,bool>> regions_; // List of address regions accessible and whether they are cacheable or not

    MemEventInitEndpoint() {}
public:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        MemEventInit::serialize_order(ser);
        SST_SER(type_);
        SST_SER(name_);
        SST_SER(regions_);
    }

    ImplementSerializable(SST::MemHierarchy::MemEventInitEndpoint);
};

class MemEventInitRegion : public MemEventInit {
public:

    enum class ReachableGroup { Source, Dest, Peer, Unknown };

    MemEventInitRegion(std::string src, MemRegion region, ReachableGroup group = ReachableGroup::Unknown) :
        MemEventInit(src, InitCommand::Region), region_(region), group_(group) { }

    MemRegion getRegion() { return region_; }

    ReachableGroup getGroup() { return group_; }
    void setGroup(ReachableGroup group) { group_ = group; }

    virtual MemEventInitRegion* clone(void) override {
        return new MemEventInitRegion(*this);
    }

    virtual std::string getVerboseString(int level = 1) override {
        std::string groupstr = "Unknown";
        if (group_ == ReachableGroup::Source) groupstr = "Source";
        else if (group_ == ReachableGroup::Dest) groupstr = "Dest";
        else if (group_ == ReachableGroup::Peer) groupstr = "Peer";
        return MemEventInit::getVerboseString(level) + region_.toString() + " Group: " + groupstr.c_str();
    }

private:
    MemRegion region_;  // MemRegion for source
    ReachableGroup group_; // Whether sent from a source/dest/peer or something else (unknown)

    MemEventInitRegion() {} // For serialization only

public:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        MemEventInit::serialize_order(ser);
        SST_SER(region_);
        SST_SER(group_);
    }

    ImplementSerializable(SST::MemHierarchy::MemEventInitRegion);
};

class MemEventUntimedFlush : public MemEventInit {
public:

    MemEventUntimedFlush(std::string src, bool request = true ) :
        MemEventInit(src, InitCommand::Flush), request_(request) { }

    virtual MemEventUntimedFlush* clone(void) override {
        return new MemEventUntimedFlush(*this);
    }

    virtual std::string getVerboseString(int level = 1) override {
        return MemEventInit::getVerboseString(level) + (request_ ? "Request" : "Response");
    }

    bool request() { return request_; }
    void setRequest(bool request) { request_ = request; }

private:
    MemEventUntimedFlush() {} // For serialization only

    bool request_; // True=request; False=response

public:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        MemEventInit::serialize_order(ser);
        SST_SER(request_);
    }

    ImplementSerializable(SST::MemHierarchy::MemEventUntimedFlush);
};


}}


#endif /* MEMHIERARCHY_MEMEVENTBASE_H */
