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

#ifndef MEMHIERARHCY_MEMEVENT_H
#define MEMHIERARHCY_MEMEVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/event.h>
#include "sst/core/element.h"

#include "util.h"

namespace SST { namespace MemHierarchy {

using namespace std;

/*
 *  Command types
 *  Not all coherence protocols use all types
 *
 *  Command, ResponseCmd, BasicCommandClass, CommandClass, cpuSideRequest, Writeback
 */
#define X_TYPES \
    X(NULLCMD,      NULLCMD,        Request,    Request,        1, 0)   /* Dummy command */\
    /* Requests */ \
    X(GetS,         GetSResp,       Request,    Request,        1, 0)   /* Read:  Request to get cache line in S state */\
    X(GetX,         GetXResp,       Request,    Request,        1, 0)   /* Write: Request to get cache line in M state */\
    X(GetSEx,       GetSResp,       Request,    Request,        1, 0)   /* Read:  Request to get cache line in M state with a LOCK flag. Invalidates will block until LOCK flag is lifted */\
                                                                        /*        GetSEx sets the LOCK, GetX removes the LOCK  */\
    X(FlushLine,    FlushLineResp,  Request,    Request,        1, 0)   /* Request to flush a cache line */\
    X(FlushLineInv, FlushLineResp,  Request,    Request,        1, 0)   /* Request to flush and invalidate a cache line */\
    X(FlushAll,     FlushAllResp,   Request,    Request,        1, 0)   /* Request to flush entire cache - similar to wbinvd */\
    /* Request Responses */\
    X(GetSResp,     NULLCMD,        Response,   Data,           0, 0)   /* Response to a GetS request */\
    X(GetXResp,     NULLCMD,        Response,   Data,           0, 0)   /* Response to a GetX request */\
    X(FlushLineResp,NULLCMD,        Response,   Ack,            0, 0)   /* Response to FlushLine request */\
    X(FlushAllResp, NULLCMD,        Response,   Ack,            0, 0)   /* Response to FlushAll request */\
    /* Writebacks, these commands also serve as invalidation acknowledgments */\
    X(PutS,         AckPut,         Request,    Request,        1, 1)   /* Clean replacement from S->I:      Remove sharer */\
    X(PutM,         AckPut,         Request,    Request,        1, 1)   /* Dirty replacement from M/O->I:    Remove owner and writeback data */\
    X(PutE,         AckPut,         Request,    Request,        1, 1)   /* Clean replacement from E->I:      Remove owner but don't writeback data */\
    /* Invalidates - sent by caches or directory controller */\
    X(Inv,          AckInv,         Request,    ForwardRequest, 0, 0)   /* Other write request:  Invalidate cache line */\
    /* Invalidates - sent by directory controller */\
    X(Fetch,        FetchResp,      Request,    ForwardRequest, 0, 0)   /* Other read request to sharer:  Get data but don't invalidate cache line */\
    X(FetchInv,     FetchResp,      Request,    ForwardRequest, 0, 0)   /* Other write request to owner:  Invalidate cache line */\
    X(FetchInvX,    FetchXResp,     Request,    ForwardRequest, 0, 0)   /* Other read request to owner:   Downgrade cache line to O/S (Remove exclusivity) */\
    X(FetchResp,    NULLCMD,        Response,   Data,           0, 0)   /* response to a Fetch, FetchInv or FetchInvX request */\
    X(FetchXResp,   NULLCMD,        Response,   Data,           0, 0)   /* response to a FetchInvX request - indicates a shared copy of the line was kept */\
    /* Others */\
    X(NACK,         NULLCMD,        Response,   Ack,            0, 0)   /* NACK response to a message */\
    X(AckInv,       NULLCMD,        Response,   Ack,            0, 0)   /* Acknowledgement response to an invalidation request */\
    X(AckPut,       NULLCMD,        Response,   Ack,            0, 0)   /* Acknowledgement response to a replacement (Put*) request */\
    X(LAST_CMD,     NULLCMD,        Request,    Request,        0, 0)

/** Valid commands for the MemEvent */
enum Command {
#define X(a,b,c,d,e,f) a,
    X_TYPES
#undef X
};

/** Response commands for MemEvents */
static const Command MemCommandResponse[] = {
#define X(a,b,c,d,e,f) b,
    X_TYPES
#undef X
};

/** Get basic command class (request or response) */
static const BasicCommandClass MemBasicCommandClass[] = {
#define X(a,b,c,d,e,f) BasicCommandClass::c,
    X_TYPES
#undef X
};

/** Get complete command type (defined in util.h) */
static const CommandClass MemCommandClass[] = {
#define X(a,b,c,d,e,f) CommandClass::d,
    X_TYPES
#undef X
};

static const bool MemCommandCPUSide[] = {
#define X(a,b,c,d,e,f) e,
    X_TYPES
#undef X
};

static const bool MemCommandWriteback[] = {
#define X(a,b,c,d,e,f) f,
    X_TYPES
#undef X
};

/** Array of the stringify'd version of the MemEvent Commands.  Useful for printing. */
static const char* CommandString[] __attribute__((unused)) = {
#define X(a,b,c,d,e,f) #a ,
    X_TYPES
#undef X
};

// statistics for the network memory inspector
static const ElementInfoStatistic networkMemoryInspector_statistics[] = {
#define X(a,b,c,d,e,f) { #a, #a, "memEvents", 1},
    X_TYPES
#undef X
    { NULL, NULL, NULL, 0 }
};

#undef X_TYPES


/* Coherence states 
 * Not all protocols use all states 
 */
#define STATE_TYPES \
    X(NP)    /* Invalid */\
    X(I)    /* Invalid */\
    X(S)    /* Shared */\
    X(E)    /* Exclusive, clean */\
    X(O)    /* Owned, dirty */\
    X(M)    /* Exclusive, dirty */\
    X(IS)   /* Invalid, have issued read request */\
    X(IM)   /* Invalid, have issued write request */\
    X(SM)   /* Shared, have issued upgrade request */\
    X(OM)   /* Owned, have issued upgrade request */\
    X(I_d)  /* I, waiting for dir entry from memory */\
    X(S_d)  /* S, waiting for dir entry from memory */\
    X(M_d)  /* M, waiting for dir entry from memory */\
    X(M_Inv)    /* M, waiting for FetchResp from owner */\
    X(M_InvX)   /* M, waiting for FetchXResp from owner */\
    X(E_Inv)    /* E, waiting for FetchResp from owner */\
    X(E_InvX)   /* E, waiting for FetchXResp from owner */\
    X(S_D)      /* S, waiting for data from memory for another GetS request */\
    X(E_D)      /* E with sharers, waiting for data from memory for another GetS request */\
    X(M_D)      /* M with sharers, waiting for data from memory for another GetS request */\
    X(SM_D)     /* SM, waiting for data from memory for another GetS request */\
    X(S_Inv)    /* S, waiting for Invalidation acks from sharers */\
    X(SM_Inv)   /* SM, waiting for Invalidation acks from sharers */\
    X(MI) \
    X(EI) \
    X(SI) \
    X(S_B)      /* S, blocked while waiting for a response (currently used for flushes) */\
    X(I_B)      /* I, blocked while waiting for a response (currently used for flushes) */\
    X(SB_Inv)   /* Was in S_B, got an Inv, resolving Inv first */\
    X(NULLST)

typedef enum {
#define X(x) x,
    STATE_TYPES
#undef X
} State;

/** Array of the stringify'd version of the MemEvent Commands.  Useful for printing. */
static const char* StateString[] __attribute__((unused)) = {
#define X(x) #x ,
    STATE_TYPES
#undef X
};

#undef STATE_TYPES

static const std::string NONE = "None";

/**
 * Interface Event used to represent Memory-based communication.
 *
 * This class primarily consists of a Command to perform at a particular address,
 * potentially including data.
 *
 * The command list includes the needed commands to execute cache coherence protocols
 * as well as standard reads and writes to memory.
 */
class MemEvent : public SST::Event  {
public:
    static const uint32_t F_LOCKED        = 0x00000001;  /* Used in a Read-Lock, Write-Unlock atomicity scheme */
    static const uint32_t F_NONCACHEABLE  = 0x00000010;  /* Used to specify that this memory event should not be cached */
    static const uint32_t F_LLSC          = 0x00000100;  /* Load Link / Store Conditional */
    static const uint32_t F_SUCCESS       = 0x00001000;  /* Indicates a successful response (used for flushes, TODO use for LLSC) */
    static const uint32_t F_NORESPONSE    = 0x00010000;

    typedef std::vector<uint8_t> dataVec;       /** Data Payload type */

    /** Creates a new MemEvent - Generic */
    MemEvent(const Component *src, Addr addr, Addr baseAddr, Command cmd) : SST::Event() {
        initialize(src->getName(), addr, baseAddr, cmd, src->getCurrentSimTimeNano());
    }

    /** MemEvent constructor - Reads */
    MemEvent(const Component *src, Addr addr, Addr baseAddr, Command cmd, uint32_t size) : SST::Event() {
        initialize(src->getName(), addr, baseAddr, cmd, src->getCurrentSimTimeNano(), size);
    }

    /** MemEvent constructor - Writes */
    MemEvent(const Component *src, Addr addr, Addr baseAddr, Command cmd, std::vector<uint8_t>& data) : SST::Event() {
        initialize(src->getName(), addr, baseAddr, cmd, src->getCurrentSimTimeNano(), data);
    }

    /** Create a new MemEvent instance, pre-configured to act as a NACK response */
    MemEvent* makeNACKResponse(MemEvent* NACKedEvent, SimTime_t timeInNano) {
        MemEvent *me      = new MemEvent(*this);
        me->responseToID_ = eventID_;
        me->dst_          = src_;
        me->NACKedEvent_  = NACKedEvent;
        me->cmd_          = NACK;
        me->initTime_     = timeInNano;
        me->rqstr_        = rqstr_;
        me->instPtr_      = instPtr_;
        me->vAddr_        = vAddr_;
        return me;
    }

    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse() {
        MemEvent *me      = new MemEvent(*this);
        me->cmd_          = MemCommandResponse[cmd_];
        me->responseToID_ = eventID_;
        me->dst_          = src_;
        me->src_          = dst_;
        me->rqstr_        = rqstr_;
        me->prefetch_     = prefetch_;
        me->instPtr_      = instPtr_;
        me->vAddr_        = vAddr_;
        me->memFlags_     = memFlags_;
        return me;
    }

    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse(State state) {
        MemEvent *me = makeResponse();
        me->setGrantedState(state);
        return me;
    }

    void initialize(std::string name, Addr addr, Addr baseAddr, Command cmd, SimTime_t timeInNano) {
        initialize();
        src_  = name;
        addr_ = addr;
        baseAddr_ = baseAddr;
        cmd_  = cmd;
        initTime_ = timeInNano;
     }

     void initialize(std::string name, Addr addr, Addr baseAddr, Command cmd, SimTime_t timeInNano, uint32_t size) {
        initialize();
        src_      = name;
        addr_     = addr;
        baseAddr_ = baseAddr;
        cmd_      = cmd;
        size_     = size;
        initTime_ = timeInNano;
     }

    void initialize(std::string name, Addr addr, Addr baseAddr, Command cmd, SimTime_t timeInNano, std::vector<uint8_t>& data) {
        initialize();
        src_        = name;
        addr_       = addr;
        baseAddr_   = baseAddr;
        cmd_        = cmd;
        initTime_   = timeInNano;
        setPayload(data);
    }

    void initialize() {
        addr_               = 0;
        cmd_                = NULLCMD;
        eventID_            = generateUniqueId();
        responseToID_       = NO_ID;
        baseAddr_           = 0;
        dst_                = NONE;
        src_                = NONE;
        rqstr_              = NONE;
        size_               = 0;
        flags_              = 0;
        memFlags_           = 0;
        prefetch_           = false;
        grantedState_       = NULLST;
        NACKedEvent_        = NULL;
        retries_            = 0;
        blocked_            = false;
        initTime_           = 0;
        payload_.clear();
        dirty_              = false;
	instPtr_	    = 0;
	vAddr_		    = 0;
        inProgress_         = false;
    }

    /** return the original event that caused a NACK */
    MemEvent* getNACKedEvent() { return NACKedEvent_; }
    /** @return  Unique ID of this MemEvent */
    id_type getID(void) const { return eventID_; }
    /** @return  Unique ID of the MemEvent that this is a response to */
    id_type getResponseToID(void) const { return responseToID_; }
    /** @return  Command of this MemEvent */
    Command getCmd(void) const { return cmd_; }
    /** Sets the Command of this MemEvent */
    void setCmd(Command newcmd) { cmd_ = newcmd; }
    /** @return  the target Address of this MemEvent */
    Addr getAddr(void) const { return addr_; }
    /** Sets the target Address of this MemEvent */
    void setAddr(Addr addr) { addr_ = addr; }
    /** Sets the Base Address of this MemEvent */
    void setBaseAddr(Addr baseAddr) { baseAddr_ = baseAddr; }

    /** Sets the virtual address of this MemEvent */
    void setVirtualAddress(Addr newVA) { vAddr_ = newVA; }
    /** Gets the virtual address of this MemEvent */
    uint64_t getVirtualAddress() { return vAddr_; }

    /** Sets the instruction pointer of that caused this MemEvent */
    void setInstructionPointer(Addr newIP) { instPtr_ = newIP; }
    /** Get the instruction pointer of that caused this MemEvent */
    uint64_t getInstructionPointer() { return instPtr_; }

    /** Returns the time (in nanoseconds) when this event was created */
    SimTime_t getInitializationTime(void) const { return initTime_; }

    /** @return  the size in bytes that this MemEvent represents */
    uint32_t getSize(void) const { return size_; }
    /** Sets the size in bytes that this MemEvent represents */
    void setSize(uint32_t size) { size_ = size; }
   
    /** Increments the number of retries */
    void incrementRetries() { retries_++; }
    int getRetries() { return retries_; }

    bool blocked() { return blocked_; }
    void setBlocked(bool value) { blocked_ = value; }
    
    bool inProgress() { return inProgress_; }
    void setInProgress(bool value) { inProgress_ = value; }

    void setLoadLink() { setFlag(MemEvent::F_LLSC); }
    bool isLoadLink() { return cmd_ == GetS && queryFlag(MemEvent::F_LLSC); }
    
    void setStoreConditional() { setFlag(MemEvent::F_LLSC); }
    bool isStoreConditional() { return cmd_ == GetX && queryFlag(MemEvent::F_LLSC); }
    
    void setSuccess(bool b) { b ? setFlag(MemEvent::F_SUCCESS) : clearFlag(MemEvent::F_SUCCESS); }
    bool success() { return queryFlag(MemEvent::F_SUCCESS); }

    bool fromHighNetNACK()  { return !MemCommandCPUSide[cmd_];}
    bool fromLowNetNACK()   { return MemCommandCPUSide[cmd_];}

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

    uint32_t getPayloadSize() {
        return payload_.size();
    }

    /** Sets the Granted State */
    void setGrantedState(State state) { grantedState_ = state;}
    /** Return the Granted State */
    State getGrantedState() { return grantedState_; }

    /** Sets that this is a prefetch command */
    void setPrefetchFlag(bool prefetch) { prefetch_ = prefetch;}
    /** Returns true if this is a prefetch command */
    bool isPrefetch() { return prefetch_; }
    
// Information about command types
    /** Returns true if this is a request that needs to access the data array (Get/Put/Flush) */
    bool isDataRequest(void) const { return MemCommandClass[cmd_] == CommandClass::Request; }
    /** Returns true if this is of response type */
    bool isResponse(void) const { return MemBasicCommandClass[cmd_] == BasicCommandClass::Response; }
    /** Returns true if this is a writeback */
    bool isWriteback(void) const { return MemCommandWriteback[cmd_]; }
    /** Returns true if this is a CPU-side event (i.e., sent from CPU side of hierarchy) */
    bool isCPUSideEvent(void) const { return MemCommandCPUSide[cmd_]; }


    void setDirty(bool status) { dirty_ = status; }
    bool getDirty() { return dirty_; }

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

    /** @returns the state of all flags for this MemEvent */
    uint32_t getFlags(void) const { return flags_; }
    /** Sets the specified flag.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent */
    void setFlag(uint32_t flag) { flags_ = flags_ | flag; }
    /** Clears the speficied flag.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent */
    void clearFlag(uint32_t flag) { flags_ = flags_ & (~flag); }
    /** Clears all flags */
    void clearFlags(void) { flags_ = 0; }
    /** Check to see if a flag is set.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent
     * @returns TRUE if the flag is set, FALSE otherwise
     */
    bool queryFlag(uint32_t flag) const { return flags_ & flag; };
    /** Sets the entire flag state */
    void setFlags(uint32_t flags) { flags_ = flags; }

    void setMemFlags(uint32_t flags) { memFlags_ = flags; }
    uint32_t getMemFlags() { return memFlags_; }

    /** Return the BaseAddr */
    Addr getBaseAddr() { return baseAddr_; }
    
private:
    id_type         eventID_;           // Unique ID for this event
    id_type         responseToID_;      // For responses, holds the ID to which this event matches
    uint32_t        flags_;             // Any flags (atomic, noncacheabel, etc.)
    uint32_t        memFlags_;          // Memory flags - ignored by caches except to be copied through. Faciliates processor-memory communication
    uint32_t        size_;              // Size in bytes that are being requested
    Addr            addr_;              // Address
    Addr            baseAddr_;          // Base (line) address
    string          src_;               // Source ID
    string          dst_;               // Destination ID
    string          rqstr_;             // Cache that originated this request
    Command         cmd_;               // Command
    MemEvent*       NACKedEvent_;       // For a NACK, pointer to the NACKed event
    int             retries_;           // For NACKed events, how many times a retry has been sent
    dataVec         payload_;           // Data
    State           grantedState_;      // For data responses, the cohrence state that the request is granted in
    bool            prefetch_;          // Whether this request came from a prefetcher
    bool            blocked_;           // Whether this request blocked for another pending request (for profiling) TODO move to mshrs
    SimTime_t       initTime_;          // Timestamp when event was created, for detecting timeouts TODO move to mshrs
    bool            dirty_;             // For a replacement, whether the data is dirty or not
    Addr	    instPtr_;           // Instruction pointer associated with the request
    Addr 	    vAddr_;             // Virtual address associated with the request
    bool            inProgress_;        // Whether this request is currently being handled, if in MSHR TODO move to mshrs

    MemEvent() {} // For serialization only

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & eventID_;
        ser & responseToID_;
        ser & flags_;
        ser & memFlags_;
        ser & size_;
        ser & addr_;
        ser & baseAddr_;
        ser & src_;
        ser & dst_;
        ser & rqstr_;
        ser & cmd_;
        ser & NACKedEvent_;
        ser & retries_;
        ser & payload_;
        ser & grantedState_;
        ser & prefetch_;
        ser & blocked_;
        ser & initTime_;
        ser & dirty_;
        ser & instPtr_;
        ser & vAddr_;
        ser & inProgress_;
    }
     
    ImplementSerializable(SST::MemHierarchy::MemEvent);     
};

}}

#endif /* INTERFACES_MEMEVENT_H */
