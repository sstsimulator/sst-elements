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

using namespace std;
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
    static const uint32_t F_LOCKED    = 0x00000001;  /* Used in a Read-Lock, Write-Unlock atomicity scheme */
    static const uint32_t F_UNCACHED  = 0x00000010;  /* Used to specify that this memory event should not be cached */
    static const uint32_t F_LLSC      = 0x00000100;  /* Load Link / Store Conditional */


    
    typedef std::vector<uint8_t> dataVec;       /** Data Payload type */

    /** Creates a new MemEvent - Genetic */
    MemEvent(const Component *_src, Addr _addr, Addr _baseAddr, Command _cmd) : SST::Event(){
        initialize(_src, _addr, _baseAddr, _cmd);
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
        MemEvent *me      = new MemEvent(*this);
        me->responseToID_ = eventID_;
        me->dst_          = src_;
        me->NACKedEvent_  = NACKedEvent;
        me->NACKedCmd_    = NACKedEvent->cmd_;
        me->cmd_          = NACK;
        return me;
    }
    
    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse(){
        MemEvent *me      = new MemEvent(*this);
        me->cmd_          = commandResponse(cmd_);
        me->responseToID_ = eventID_;
        me->dst_          = src_;
        me->src_          = dst_;
        return me;
    }

    /** Generate a new MemEvent, pre-populated as a response */
    MemEvent* makeResponse(BCC_MESIState state){
        MemEvent *me = makeResponse();
        me->setGrantedState(state);
        return me;
    }
    
    
    void initialize(const Component *_src, Addr _addr, Addr _baseAddr, Command _cmd){
        initialize();
        src_  = _src->getName();
        addr_ = _addr;
        baseAddr_ = _baseAddr;
        cmd_  = _cmd;
     }
    
     void initialize(const Component *_src, Addr _addr, Addr _baseAddr, Command _cmd, uint32_t _size){
        initialize();
        src_      = _src->getName();
        addr_     = _addr;
        baseAddr_ = _baseAddr;
        cmd_      = _cmd;
        size_     = _size;
     
     }
    
    void initialize(const Component *_src, Addr _addr, Addr _baseAddr, Command _cmd, std::vector<uint8_t>& _data){
        initialize();
        src_         = _src->getName();
        addr_        = _addr;
        baseAddr_    = _baseAddr;
        cmd_         = _cmd;
        setPayload(_data);
    }
    
    void initialize(){
        addr_             = 0;
        cmd_              = NULLCMD;
        eventID_          = generateUniqueId();
        responseToID_     = NO_ID;
        baseAddr_         = 0;
        dst_              = NONE;
        src_              = NONE;
        size_             = 0;
        flags_            = 0;
        groupID_          = 0;
        prefetch_         = false;
        ackNeeded_        = false;
        atomic_           = false;
        loadLink_         = false;
        storeConditional_ = false;
        grantedState_     = NULLST;
        startTime_        = 0;
        NACKedCmd_        = NULLCMD;
        NACKedEvent_      = NULL;
        payload_.clear();
    }


    /** return the original event that caused a NACK */
    MemEvent* getNACKedEvent() { return NACKedEvent_; }
    /** return the original command type that caused a NACK */
    Command getNACKedCmd() { return NACKedCmd_; }
    /** @return  Unique ID of this MemEvent */
    id_type getID(void) const { return eventID_; }
    /** @return  Unique ID of the MemEvent that this is a response to */
    id_type getResponseToID(void) const { return responseToID_; }
    /** @return  Command of this MemEvent */
    Command getCmd(void) const { return cmd_; }
    /** Sets the Command of this MemEvent */
    void setCmd(Command _newcmd) { cmd_ = _newcmd; }
    /** @return  the target Address of this MemEvent */
    Addr getAddr(void) const { return addr_; }
    /** Sets the target Address of this MemEvent */
    void setAddr(Addr _addr) { addr_ = _addr; }
    /** Sets the Base Address of this MemEvent */
    void setBaseAddr(Addr _baseAddr) { baseAddr_ = _baseAddr; }

    /** @return  the size in bytes that this MemEvent represents */
    uint32_t getSize(void) const { return size_; }
    /** Sets the size in bytes that this MemEvent represents */
    void setSize(uint32_t _size) {
        size_ = _size;
    }
    
    
    void setLoadLink(){ loadLink_ = true; }
    bool isLoadLink() { return loadLink_; }
    
    void setStoreConditional(){ storeConditional_ = true;}
    bool isStoreConditional(){ return storeConditional_; }
    
    void setAtomic(bool b){ b ? setFlag(MemEvent::F_LLSC) : clearFlag(MemEvent::F_LLSC); }
    bool isAtomic(){ return queryFlag(MemEvent::F_LLSC); }
    
    bool isHighNetEvent(){
        if(cmd_ == GetS || cmd_ == GetX || cmd_ == GetSEx){
            return true;
        }
        return false;
    }
    
   bool isLowNetEvent(){
        if(cmd_ == Inv || cmd_ == InvX ||
           cmd_ == FetchInv || cmd_ == FetchInvX){
            return true;
        }
        return false;
    }
    
    bool isWriteback(){
        if(cmd_ == PutS || cmd_ == PutM ||
           cmd_ == PutE || cmd_ == PutX || cmd_ == PutXE){
            return true;
        }
        return false;
    
    }
    
    bool fromHighNetNACK(){ return isLowNetEvent();}
    bool fromLowNetNACK(){ return isHighNetEvent();}

    /** @return  the data payload. */
    dataVec& getPayload(void){
        /* Lazily allocate space for payload */
        if ( payload_.size() < size_ )  payload_.resize(size_);
        return payload_;
    }
    

    /** Sets the data payload and payload size.
     * @param[in] data  Vector from which to copy data
     */
    void setPayload(std::vector<uint8_t>& _data) {
        payload_ = _data;
    }
    
    /** Sets the data payload and payload size.
     * @param[in] size  How many bytes to copy from data
     * @param[in] data  Data array to set as payload
     */
    void setPayload(uint32_t _size, uint8_t* _data){
        setSize(_size);
        payload_.resize(_size);
        for ( uint32_t i = 0 ; i < _size ; i++ ) {
            payload_[i] = _data[i];
        }
    }

    /** Sets the Granted State */
    void setGrantedState(BCC_MESIState _state){ grantedState_ = _state;}
    /** Return the Granted State */
    BCC_MESIState getGrantedState(){ return grantedState_; }

    /** Sets that this is a prefetch command */
    void setPrefetchFlag(bool _prefetch){ prefetch_ = _prefetch;}
    /** Returns true if this is a prefetch command */
    bool isPrefetch(){ return prefetch_; }
    
    /** Returns true if this is a Data Request */
    static bool isDataRequest(Command cmd){ return (cmd == GetS || cmd == GetX || cmd == GetSEx || cmd == FetchInv || cmd == FetchInvX); }
    /** Returns true if this is of cpu type */
    static bool isCPURequest(Command cmd){ return (cmd == GetS || cmd == GetX || cmd == GetSEx);}
    /** Returns true if this is of response type */
    static bool isResponse(Command cmd){ return (cmd == GetSResp || cmd == GetXResp);}
    /** Returns true if this is a 'writeback' command type */
    static bool isWriteback(Command cmd){ return (cmd == PutM || cmd == PutE || cmd == PutX || cmd == PutXE || cmd == PutS); }
    

    
    /** Setter for GroupId */
    void setGroupId(uint32_t _groupID){ groupID_ = _groupID; }
    /** Getter for GroupId */
    uint32_t getGroupId() { return groupID_; }
    
    /** Set ackNeeded member variable */
    void setAckNeeded(){ ackNeeded_ = true;}
     /** Getter for ackNeeded member variable */
    bool getAckNeeded(){ return ackNeeded_;}


    /** @return the source string - who sent this MemEvent */
    const std::string& getSrc(void) const { return src_; }
    /** Sets the source string - who sent this MemEvent */
    void setSrc(const std::string& _src) { src_ = _src; }
    /** @return the destination string - who receives this MemEvent */
    const std::string& getDst(void) const { return dst_; }
    /** Sets the destination string - who received this MemEvent */
    void setDst(const std::string& _d) { dst_ = _d; }

    /** @returns the state of all flags for this MemEvent */
    uint32_t getFlags(void) const { return flags_; }
    /** Sets the specified flag.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent */
    void setFlag(uint32_t _flag) { flags_ = flags_ | _flag; }
    /** Clears the speficied flag.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent */
    void clearFlag(uint32_t _flag) { flags_ = flags_ & (~_flag); }
    /** Clears all flags */
    void clearFlags(void) { flags_ = 0; }
    /** Check to see if a flag is set.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent
     * @returns TRUE if the flag is set, FALSE otherwise
     */
    bool queryFlag(uint32_t _flag) const { return flags_ & _flag; };
    /** Sets the entire flag state */
    void setFlags(uint32_t _flags) { flags_ = _flags; }

    /** Return the BaseAddr */
    Addr getBaseAddr(){ return baseAddr_; }
    
    void setStartTime(uint64_t _startTime) { startTime_ = _startTime; }
    uint64_t getStartTime(){return startTime_;}

    /** Return the command that is the Response to the input command */
    static Command commandResponse(Command _c){
        switch(_c) {
            case GetS:
            case GetSEx:
                return GetSResp;
            case GetX:
                return GetXResp;
            case FetchInv:
            case FetchInvX:
                return FetchResp;
            default:
                return NULLCMD;
        }
    }
private:
    id_type         eventID_;
    id_type         responseToID_;
    uint32_t        flags_;
    uint32_t        size_;
    uint32_t        groupID_;
    Addr            addr_;
    Addr            baseAddr_;
    string          src_;
    string          dst_;
    Command         cmd_;
    Command         NACKedCmd_;
    MemEvent*       NACKedEvent_;
    dataVec         payload_;
    BCC_MESIState   grantedState_;
    bool            ackNeeded_;
    bool            prefetch_;
    bool            atomic_;
    bool            loadLink_;
    bool            storeConditional_;
    uint64_t        startTime_;

    
    MemEvent() {} // For serialization only

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(eventID_);
        ar & BOOST_SERIALIZATION_NVP(responseToID_);
        ar & BOOST_SERIALIZATION_NVP(addr_);
        ar & BOOST_SERIALIZATION_NVP(size_);
        ar & BOOST_SERIALIZATION_NVP(cmd_);
        ar & BOOST_SERIALIZATION_NVP(payload_);
        ar & BOOST_SERIALIZATION_NVP(src_);
        ar & BOOST_SERIALIZATION_NVP(dst_);
        ar & BOOST_SERIALIZATION_NVP(flags_);
    }
};

}}

#endif /* INTERFACES_MEMEVENT_H */
