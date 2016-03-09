// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARHCY_MEMEVENT_H
#define MEMHIERARHCY_MEMEVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>
#include <sst/core/component.h>
#include <sst/core/event.h>
#include "sst/core/element.h"


namespace SST { namespace MemHierarchy {

using namespace std;
typedef uint64_t Addr;

/*
 *  Command types
 *  Not all coherence protocols use all types
 */
#define X_TYPES \
    X(NULLCMD)         /* Dummy command */\
    /* Requests */ \
    X(GetS)            /* Read:  Request to get cache line in S state */\
    X(GetX)            /* Write: Request to get cache line in M state */\
    X(GetSEx)          /* Read:  Request to get cache line in M state with a LOCK flag. Invalidates will block until LOCK flag is lifted */\
                       /*        GetSEx sets the LOCK, GetX removes the LOCK  */\
    /* Request Responses */\
    X(GetSResp)        /* Response to a GetS request */\
    X(GetXResp)        /* Response to a GetX request */\
    /* Writebacks, these commands also serve as invalidation acknowledgments */\
    X(PutS)            /* Clean replacement from S->I:      Remove sharer */\
    X(PutM)            /* Dirty replacement from M/O->I:    Remove owner and writeback data */\
    X(PutE)            /* Clean replacement from E->I:      Remove owner but don't writeback data */\
    X(PutX)            /* Dirty downgrade from M->O/S:      Remove exclusive ownership, add as sharer (MSI/MESI) or non-exclusive owner (MOESI), writeback data */\
    X(PutXE)           /* Clean downgrade from E->O/S:      Remove exclusive ownership, add as sharer (MSI/MESI) or non-exclusive owner (MOESI), don't writeback data */\
    /* Invalidates - sent by caches or directory controller */\
    X(Inv)             /* Other write request:  Invalidate cache line */\
    /* Invalidates - sent by directory controller */\
    X(Fetch)           /* Other read request to sharer:  Get data but don't invalidate cache line */\
    X(FetchInv)        /* Other write request to owner:  Invalidate cache line */\
    X(FetchInvX)       /* Other read request to owner:   Downgrade cache line to O/S (Remove exclusivity) */\
    X(FetchResp)       /* response to a Fetch, FetchInv or FetchInvX request */\
    X(FetchXResp)      /* response to a FetchInvX request - indicates a shared copy of the line was kept */\
    /* Others */\
    X(NACK)\
    X(AckInv)\
    X(AckPut) \
    X(LAST_CMD)

/** Valid commands for the MemEvent */
typedef enum {
#define X(x) x,
    X_TYPES
#undef X
} Command;

/** Array of the stringify'd version of the MemEvent Commands.  Useful for printing. */
static const char* CommandString[] __attribute__((unused)) = {
#define X(x) #x ,
    X_TYPES
#undef X
};

// statistics for the network memory inspector
static const ElementInfoStatistic networkMemoryInspector_statistics[] = {
#define X(x) { #x, #x, "memEvents", 1},
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
class MemEvent : public SST::Event {
public:
    static const uint32_t F_LOCKED        = 0x00000001;  /* Used in a Read-Lock, Write-Unlock atomicity scheme */
    static const uint32_t F_NONCACHEABLE  = 0x00000010;  /* Used to specify that this memory event should not be cached */
    static const uint32_t F_LLSC          = 0x00000100;  /* Load Link / Store Conditional */

    typedef std::vector<uint8_t> dataVec;       /** Data Payload type */

    /** Creates a new MemEvent - Generic */
    MemEvent(const Component *src, Addr addr, Addr baseAddr, Command cmd) : SST::Event() {
        initialize(src, addr, baseAddr, cmd);
    }

    /** MemEvent constructor - Reads */
    MemEvent(const Component *src, Addr addr, Addr baseAddr, Command cmd, uint32_t size) : SST::Event() {
        initialize(src, addr, baseAddr, cmd, size);
    }

    /** MemEvent constructor - Writes */
    MemEvent(const Component *src, Addr addr, Addr baseAddr, Command cmd, std::vector<uint8_t>& data) : SST::Event() {
        initialize(src, addr, baseAddr, cmd, data);
    }

    /** Create a new MemEvent instance, pre-configured to act as a NACK response */
    MemEvent* makeNACKResponse(const Component *source, MemEvent* NACKedEvent) {
        MemEvent *me      = new MemEvent(*this);
        me->responseToID_ = eventID_;
        me->dst_          = src_;
        me->NACKedEvent_  = NACKedEvent;
        me->cmd_          = NACK;
        me->initTime_     = source->getCurrentSimTimeNano();
        me->rqstr_        = rqstr_;
        me->instPtr_      = instPtr_;
        me->vAddr_        = vAddr_;
        return me;
    }

    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse() {
        MemEvent *me      = new MemEvent(*this);
        me->cmd_          = commandResponse(cmd_);
        me->responseToID_ = eventID_;
        me->dst_          = src_;
        me->src_          = dst_;
        me->rqstr_        = rqstr_;
        me->prefetch_     = prefetch_;
        me->instPtr_      = instPtr_;
        me->vAddr_        = vAddr_;
        return me;
    }

    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse(State state) {
        MemEvent *me = makeResponse();
        me->setGrantedState(state);
        return me;
    }

    void initialize(const Component *src, Addr addr, Addr baseAddr, Command cmd) {
        initialize();
        src_  = src->getName();
        addr_ = addr;
        baseAddr_ = baseAddr;
        cmd_  = cmd;
        initTime_ = src->getCurrentSimTimeNano();
     }

     void initialize(const Component *src, Addr addr, Addr baseAddr, Command cmd, uint32_t size) {
        initialize();
        src_      = src->getName();
        addr_     = addr;
        baseAddr_ = baseAddr;
        cmd_      = cmd;
        size_     = size;
        initTime_ = src->getCurrentSimTimeNano();
     }

    void initialize(const Component *src, Addr addr, Addr baseAddr, Command cmd, std::vector<uint8_t>& data) {
        initialize();
        src_         = src->getName();
        addr_        = addr;
        baseAddr_    = baseAddr;
        cmd_         = cmd;
        initTime_ = src->getCurrentSimTimeNano();
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
        groupID_            = 0;
        prefetch_           = false;
        atomic_             = false;
        loadLink_           = false;
        storeConditional_   = false;
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

    void setLoadLink() { loadLink_ = true; }
    bool isLoadLink() { return loadLink_; }
    
    void setStoreConditional() { storeConditional_ = true;}
    bool isStoreConditional() { return storeConditional_; }
    
    void setAtomic(bool b) { b ? setFlag(MemEvent::F_LLSC) : clearFlag(MemEvent::F_LLSC); }
    bool isAtomic() { return queryFlag(MemEvent::F_LLSC); }
    
    bool isHighNetEvent() {
        if (cmd_ == GetS || cmd_ == GetX || cmd_ == GetSEx || isWriteback()) {
            return true;
        }
        return false;
    }
    
   bool isLowNetEvent() {
        if (cmd_ == Inv || cmd_ == FetchInv || cmd_ == FetchInvX || cmd_ == Fetch) {
            return true;
        }
        return false;
    }
    
    bool isWriteback() {
        if (cmd_ == PutS || cmd_ == PutM ||
           cmd_ == PutE || cmd_ == PutX || cmd_ == PutXE) {
            return true;
        }
        return false;
    
    }
    
    bool fromHighNetNACK() { return isLowNetEvent();}
    bool fromLowNetNACK() { return isHighNetEvent();}

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
    
    /** Returns true if this is a Data Request */
    static bool isDataRequest(Command cmd) { return (cmd == GetS || cmd == GetX || cmd == GetSEx || cmd == FetchInv || cmd == FetchInvX || cmd == Fetch); }
    bool isDataRequest(void) const { return MemEvent::isDataRequest(cmd_); }
    /** Returns true if this is of cpu type */
    static bool isCPURequest(Command cmd) { return (cmd == GetS || cmd == GetX || cmd == GetSEx);}
    bool isCPURequest(void) const { return MemEvent::isCPURequest(cmd_); }
    /** Returns true if this is of response type */
    static bool isResponse(Command cmd) { return (cmd == GetSResp || cmd == GetXResp);}
    bool isResponse(void) const { return MemEvent::isResponse(cmd_); }
    /** Returns true if this is a 'writeback' command type */
    static bool isWriteback(Command cmd) { return (cmd == PutM || cmd == PutE || cmd == PutX || cmd == PutXE || cmd == PutS); }
    bool isWriteback(void) const { return MemEvent::isWriteback(cmd_); }
   

    
    /** Setter for GroupId */
    void setGroupId(uint32_t groupID) { groupID_ = groupID; }
    /** Getter for GroupId */
    uint32_t getGroupId() { return groupID_; }
    
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

    /** Return the BaseAddr */
    Addr getBaseAddr() { return baseAddr_; }
    
    /** Return the command that is the Response to the input command */
    static Command commandResponse(Command cmd) {
        switch(cmd) {
            case GetS:
            case GetSEx:
                return GetSResp;
            case GetX:
                return GetXResp;
            case FetchInv:
            case Fetch:
                return FetchResp;
            case FetchInvX:
                return FetchXResp;
            default:
                return NULLCMD;
        }
    }

#ifdef USE_VAULTSIM_HMC
    /** Setter for HMC instruction type */
    void setHMCInstType(uint8_t hmcInstType) { hmcInstType_ = hmcInstType; }
    /** Getter for HMC instruction type */
    uint8_t getHMCInstType() { return hmcInstType_; }

private:
    uint8_t         hmcInstType_;
#endif

private:
    id_type         eventID_;           // Unique ID for this event
    id_type         responseToID_;      // For responses, holds the ID to which this event matches
    uint32_t        flags_;             // Any flags (atomic, noncacheabel, etc.)
    uint32_t        size_;              // Size in bytes that are being requested
    uint32_t        groupID_;           // ???
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
    bool            atomic_;            // Whether this request is atomic
    bool            loadLink_;          // Whether this request in a LL
    bool            storeConditional_;  // Whether this request is a SC
    bool            blocked_;           // Whether this request blocked for another pending request (for profiling)
    SimTime_t       initTime_;          // Timestamp when event was created, for detecting timeouts
    bool            dirty_;             // For a replacement, whether the data is dirty or not
    Addr	    instPtr_;           // Instruction pointer associated with the request
    Addr 	    vAddr_;             // Virtual address associated with the request
    bool            inProgress_;        // Whether this request is currently being handled, if in MSHR

    MemEvent() {} // For serialization only

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(eventID_);
        ar & BOOST_SERIALIZATION_NVP(responseToID_);
        ar & BOOST_SERIALIZATION_NVP(flags_);
        ar & BOOST_SERIALIZATION_NVP(size_);
        ar & BOOST_SERIALIZATION_NVP(groupID_);
        ar & BOOST_SERIALIZATION_NVP(addr_);
        ar & BOOST_SERIALIZATION_NVP(baseAddr_);
        ar & BOOST_SERIALIZATION_NVP(src_);
        ar & BOOST_SERIALIZATION_NVP(dst_);
        ar & BOOST_SERIALIZATION_NVP(rqstr_);
        ar & BOOST_SERIALIZATION_NVP(cmd_);
        ar & BOOST_SERIALIZATION_NVP(NACKedEvent_);
        ar & BOOST_SERIALIZATION_NVP(retries_);
        ar & BOOST_SERIALIZATION_NVP(payload_);
        ar & BOOST_SERIALIZATION_NVP(grantedState_);
        ar & BOOST_SERIALIZATION_NVP(prefetch_);
        ar & BOOST_SERIALIZATION_NVP(atomic_);
        ar & BOOST_SERIALIZATION_NVP(loadLink_);
        ar & BOOST_SERIALIZATION_NVP(storeConditional_);
        ar & BOOST_SERIALIZATION_NVP(blocked_);
        ar & BOOST_SERIALIZATION_NVP(initTime_);
        ar & BOOST_SERIALIZATION_NVP(dirty_);
        ar & BOOST_SERIALIZATION_NVP(instPtr_);
        ar & BOOST_SERIALIZATION_NVP(vAddr_);
        ar & BOOST_SERIALIZATION_NVP(inProgress_);
#ifdef USE_VAULTSIM_HMC
        ar & BOOST_SERIALIZATION_NVP(hmcInstType_);
#endif
    }
};

}}

#endif /* INTERFACES_MEMEVENT_H */
