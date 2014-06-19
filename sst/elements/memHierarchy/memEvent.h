// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
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

namespace SST { namespace MemHierarchy {

typedef uint64_t Addr;

/* Coherence states for Bottom Coherence Controller Cache Lines, MESI Protocol */
/* DO NOT CHANGE ORDERING!!!!  If ordering needs to change, change code in cacheEventProcessing.cc, cacheController::checkCacheLineIsStable   */
#define X_TYPES \
    X(NULLCMD) \
    /* Requests */ \
    X(GetS) \
    X(GetX) \
    X(GetSEx) \
    /* Request Responses */ \
    X(GetSResp) \
    X(GetXResp) \
    /* Writebacks */ \
    X(PutS) \
    X(PutM) \
    X(PutE) \
    X(PutX) \
    X(PutXE) \
    /* Invalidates */ \
    X(Inv)  \
    X(InvX) \
    /* Directory Controller*/ \
    X(FetchInv) \
    X(FetchInvX) \
    X(FetchResp) \
    /* Others */ \
    X(InvAck)  \
    X(NACK) \
    X(PutAck) \
    X(ReadReq) \
    X(ReadReqEx) \
    X(ReadResp) \
    X(WriteReq) \
    X(WriteResp) \
    /* Cache <-> Cache/MemControl/DirCtrl */ \
    X(RequestData) \
    X(SupplyData) \
    X(Invalidate) \
    X(ACK) \
    /* Directory Controller */ \
    X(Evicted)

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

#undef X_TYPES


/* Coherence states for Top Coherence Controller Cache Lines */
    
#define TCCLINE_TYPES \
    X(V) \
    X(InvX_A) \
    X(Inv_A)

/** Valid commands for the MemEvent */
typedef enum {
#define X(x) x,
    TCCLINE_TYPES
#undef X
} TCC_MESIState;

/** Array of the stringify'd version of the MemEvent Commands.  Useful for printing. */
static const char* TccLineString[] __attribute__((unused)) = {
#define X(x) #x ,
    TCCLINE_TYPES
#undef X
};

#undef TCCLINE_TYPES

#define BCCLINE_TYPES \
    X(I) \
    X(IS) \
    X(IM) \
    X(S)  \
    X(SI) \
    X(EI) \
    X(SM) \
    X(E) \
    X(M) \
    X(MI) \
    X(MS) \
    X(DUMMY) \
    X(NULLST)

typedef enum {
#define X(x) x,
    BCCLINE_TYPES
#undef X
} BCC_MESIState;

/** Array of the stringify'd version of the MemEvent Commands.  Useful for printing. */
static const char* BccLineString[] __attribute__((unused)) = {
#define X(x) #x ,
    BCCLINE_TYPES
#undef X
};

#undef BCCLINE_TYPES

//TODO: Make it more robust
static const BCC_MESIState nextState[] = {I, S, M, S, I, I, M, E, M, I, S};
static const std::string NONE = "None";

/**
 * Interface Event used to represent Memory-based communication.
 *
 * This class primarily consists of a Command to perform at a particular address,
 * potentially including data.
 *
 * The command list includes the needed commands to execute cache coherency protocols
 * as well as standard reads and writes to memory.
 */
class MemEvent : public SST::Event {
public:
    static const uint32_t F_LOCKED    = (1<<1);  /* Used in a Read-Lock, Write-Unlock atomicity scheme */
    static const uint32_t F_UNCACHED  = (1<<2);  /* Used to specify that this memory event should not be cached */
    static const uint32_t F_LLSC      = (1<<3);  /* Load Link / Store Conditional */


    
    typedef std::vector<uint8_t> dataVec;       /** Data Payload type */

    /** Creates a new MemEvent - Genetic */
    MemEvent(const Component *_src, Addr _addr, Command _cmd) : SST::Event(){
        initialize(_src, _addr, _cmd);
    }

    /** MemEvent constructor - Reads */
    MemEvent(const Component *_src, Addr _addr, Addr _baseAddr, Command _cmd, uint32_t _size) : SST::Event() {
        initialize(_src, _addr, _baseAddr, _cmd, _size);
    }

    /** MemEvent constructor - Writes */
    MemEvent(const Component *_src, Addr _addr, Addr _baseAddr, Command _cmd, std::vector<uint8_t>& _data) : SST::Event() {
        initialize(_src, _addr, _baseAddr, _cmd, _data);
    }

    /** Create a new MemEvent instance, pre-configured to act as a NACK response */
    MemEvent* makeNACKResponse(const Component *source, MemEvent* NACKedEvent){
        MemEvent *me = new MemEvent(*this);
        me->response_to_id = event_id;
        me->dst = src;
        me->prefetch = prefetch;
        me->setGrantedState(NULLST);
        me->NACKedEvent = NACKedEvent;
        me->NACKedCmd = NACKedEvent->cmd;
        return me;
    }
    
    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse(const Component *source){
        MemEvent *me = new MemEvent(*this);
        me->cmd = commandResponse(cmd);
        me->response_to_id = event_id;
        me->dst = src;
        return me;
    }

    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse(const Component *source, BCC_MESIState state){
        MemEvent *me = makeResponse(source);
        me->setGrantedState(state);
        return me;
    }
    
    
    void initialize(const Component *_src, Addr _addr, Command _cmd){
        initialize();
        src  = _src->getName();
        addr = _addr;
        cmd  = _cmd;
     }
    
     void initialize(const Component *_src, Addr _addr, Addr _baseAddr, Command _cmd, uint32_t _size){
        initialize();
        src      = _src->getName();
        addr     = _addr;
        baseAddr = _baseAddr;
        cmd      = _cmd;
        size     = _size;
     
     }
    
    void initialize(const Component *_src, Addr _addr, Addr _baseAddr, Command _cmd, std::vector<uint8_t>& _data){
        initialize();
        src         = _src->getName();
        addr        = _addr;
        baseAddr    = _baseAddr;
        cmd         = _cmd;
        setPayload(_data);
    }
    
    void initialize(){
        addr             = 0;
        cmd              = NULLCMD;
        event_id         = generateUniqueId();
        response_to_id   = NO_ID;
        baseAddr         = 0;
        dst              = NONE;
        size             = 0;
        flags            = 0;
        groupId          = 0;
        prefetch         = false;
        ackNeeded        = false;
        atomic           = false;
        loadLink         = false;
        storeConditional = false;
    }


    /** return the original command that caused a NACK */
    MemEvent* getNACKedEvent() { return NACKedEvent; }
    
    Command getNACKedCmd() { return NACKedCmd; }

    /** @return  Unique ID of this MemEvent */
    id_type getID(void) const { return event_id; }
    /** @return  Unique ID of the MemEvent that this is a response to */
    id_type getResponseToID(void) const { return response_to_id; }
    /** @return  Command of this MemEvent */
    Command getCmd(void) const { return cmd; }
    /** Sets the Command of this MemEvent */
    void setCmd(Command newcmd) { cmd = newcmd; }
    /** @return  the target Address of this MemEvent */
    Addr getAddr(void) const { return addr; }
    /** Sets the target Address of this MemEvent */
    void setAddr(Addr newAddr) { addr = newAddr; }
    /** Sets the Base Address of this MemEvent */
    void setBaseAddr(Addr newBaseAddr) { baseAddr = newBaseAddr; }

    /** @return  the size in bytes that this MemEvent represents */
    uint32_t getSize(void) const { return size; }
    /** Sets the size in bytes that this MemEvent represents */
    void setSize(uint32_t sz) {
        size = sz;
    }
    
    
    void setLoadLink(){ loadLink = true; }
    bool isLoadLink() { return loadLink; }
    
    void setStoreConditional(){ storeConditional = true;}
    bool isStoreConditional(){ return storeConditional; }
    
    void setAtomic(){ atomic = true; }
    bool isAtomic(){ return atomic; }
    
    bool isHighNetEvent(){
        if(cmd == GetS || cmd == GetX || cmd == GetSEx){
            return true;
        }
        return false;
    }
    
   bool isLowNetEvent(){
        if(cmd == Inv || cmd == InvX ||
           cmd == FetchInv || cmd == FetchInvX){
            return true;
        }
        return false;
    }
    
    bool isWriteback(){
        if(cmd == PutS || cmd == PutM ||
           cmd == PutE || cmd == PutX || cmd == PutXE){
            return true;
        }
        return false;
    
    }
    
    bool fromHighNetNACK(){ return isLowNetEvent();}
    bool fromLowNetNACK(){return isHighNetEvent();}

    /** @return  the data payload. */
    dataVec& getPayload(void)
    {
        /* Lazily allocate space for payload */
        if ( payload.size() < size )  payload.resize(size);
        return payload;
    }
    

    /** Sets the data payload and payload size.
     * @param[in] data  Vector from which to copy data
     */
    void setPayload(std::vector<uint8_t>& data) {
        setSize(data.size());
        payload = data;
    }
    
    /** Sets the data payload and payload size.
     * @param[in] size  How many bytes to copy from data
     * @param[in] data  Data array to set as payload
     */
    void setPayload(uint32_t size, uint8_t *data)
    {
        setSize(size);
        payload.resize(size);
        for ( uint32_t i = 0 ; i < size ; i++ ) {
            payload[i] = data[i];
        }
    }

    /** Sets the Granted State */
    void setGrantedState(BCC_MESIState _state){
        grantedState = _state;
    }

    /** Sets that this is a prefetch command */
    void setPrefetchFlag(bool _prefetch){
        prefetch = _prefetch;
    }

    /** Returns true if this is a prefetch command */
    bool isPrefetch(){ return prefetch; }

    /** Return the Granted State */
    BCC_MESIState getGrantedState(){ return grantedState; }


    /** Returns true if this is a Data Request */
    static bool isDataRequest(Command cmd){
        return (cmd == GetS || cmd == GetX || cmd == GetSEx || cmd == FetchInv || cmd == FetchInvX);
    }
    
    /** Returns true if this is of cpu type */
    static bool isCPURequest(Command cmd){
        return (cmd == GetS || cmd == GetX || cmd == GetSEx);
    }
    
    /** Returns true if this is of response type */
    static bool isResponse(Command cmd){
        return (cmd == GetSResp || cmd == GetXResp);
    }
    
    /** Returns true if this is a 'writeback' command type */
    static bool isWriteback(Command cmd){
        return (cmd == PutM || cmd == PutE || cmd == PutX || cmd == PutXE || cmd == PutS);
    }
    
    /** Set ackNeeded member variable */
    void setAckNeeded(){ ackNeeded = true;}
    
    /** Setter for GroupId */
    void setGroupId(uint32_t _groupId){ groupId = _groupId; }
    
    /** Getter for GroupId */
    uint32_t getGroupId() { return groupId; }
    
    /** Getter for ackNeeded member variable */
    bool getAckNeeded(){ return ackNeeded;}


    /** @return the source string - who sent this MemEvent */
    const std::string& getSrc(void) const { return src; }
    /** Sets the source string - who sent this MemEvent */
    void setSrc(const std::string &s) { src = s; }
    /** @return the destination string - who receives this MemEvent */
    const std::string& getDst(void) const { return dst; }
    /** Sets the destination string - who received this MemEvent */
    void setDst(const std::string &d) { dst = d; }

    /** @returns the state of all flags for this MemEvent */
    uint32_t getFlags(void) const { return flags; }
    /** Sets the specified flag.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent */
    void setFlag(uint32_t flag) { flags = flags | flag; }
    /** Clears the speficied flag.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent */
    void clearFlag(uint32_t flag) { flags = flags & (~flag); }
    /** Clears all flags */
    void clearFlags(void) { flags = 0; }
    /** Check to see if a flag is set.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent
     * @returns TRUE if the flag is set, FALSE otherwise
     */
    bool queryFlag(uint32_t flag) const { return flags & flag; };
    /** Sets the entire flag state */
    void setFlags(uint32_t _flags) { flags = _flags; }

    /** @returns the (optional) ID associated with the flag F_LOCKED */
    uint64_t getLockID(void) const { return lockid; }
    /** sets the (optional) ID associated with the flag F_LOCKED */
    void setLockID(uint64_t id) { lockid = id; }

    /** Return the BaseAddr */
    Addr getBaseAddr(){ return baseAddr; }
    
    void setStartTime(uint64_t sTime) { startTime = sTime; }
    
    uint64_t getStartTime(){
        return startTime;
    }

    /** Return the command that is the Response to the input command */
    static Command commandResponse(Command c)
    {
        switch(c) {
        case GetS:
        case GetSEx:
            return GetSResp;
        case GetX:
            return GetXResp;
        case FetchInv:
        case FetchInvX:
            return FetchResp;
            //return SupplyData;
        default:
            return NULLCMD;
        }
    }
private:
    id_type     event_id;
    id_type     response_to_id;
    uint64_t    lockid;
    Addr        addr;
    Addr        baseAddr;
    uint32_t    size;
    Command     cmd;
    dataVec     payload;
    
    std::string src;
    std::string dst;
    
    uint32_t    flags;
    bool        prefetch;
    BCC_MESIState grantedState;
    
    uint64_t    startTime;
    
    MemEvent*   NACKedEvent;
    Command     NACKedCmd;
    
    bool        ackNeeded;
    uint32_t    groupId;
    
    bool        atomic;
    bool        loadLink;
    bool        storeConditional;
    


    
    MemEvent() {} // For serialization only



    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(event_id);
        ar & BOOST_SERIALIZATION_NVP(response_to_id);
        ar & BOOST_SERIALIZATION_NVP(addr);
        ar & BOOST_SERIALIZATION_NVP(size);
        ar & BOOST_SERIALIZATION_NVP(cmd);
        ar & BOOST_SERIALIZATION_NVP(payload);
        ar & BOOST_SERIALIZATION_NVP(src);
        ar & BOOST_SERIALIZATION_NVP(dst);
        ar & BOOST_SERIALIZATION_NVP(flags);
    }
};

}}

#endif /* INTERFACES_MEMEVENT_H */
