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

#ifndef MEMHIERARCHY_MEMTYPES_H
#define MEMHIERARCHY_MEMTYPES_H

#include <sst/core/sst_types.h>
#include <limits>
#include <numeric>

#include <sst/core/eli/elibase.h>   // For ElementInfoStatistic
#include <sst/core/serialization/serializable.h> // For serializable MemRegion

#include "util.h"

namespace SST { 
namespace MemHierarchy {

using namespace std;


// Command attributes
enum class CommandClass { Request, Data, Ack, ForwardRequest };     // TODO - route messages on VCs based on command class
enum class BasicCommandClass {Request, Response};                   // Whether a command is a request or response
enum class MemEventType { Cache, Move, Custom };                    // For parsing which kind of event a MemEventBase is, 'Custom' is a catchall for types memH doesn't know about



/******************************************************************************************
 *  Commands used throughout MemH
 *  Not all components handle all types
 *
 *  Command, ResponseCmd, BasicCommandClass, CommandClass, routeByAddr, Writeback, EventType
 *****************************************************************************************/
#define X_CMDS \
    X(NULLCMD,          NULLCMD,        Request,    Request,        1, 0,   Cache)   /* Dummy command */\
    /* Requests */ \
    X(GetS,             GetSResp,       Request,    Request,        1, 0,   Cache)   /* Read:  Request to get cache line in S state */\
    X(GetX,             GetXResp,       Request,    Request,        1, 0,   Cache)   /* Write: Request to get cache line in M state */\
    X(GetSX,            GetXResp,       Request,    Request,        1, 0,   Cache)   /* Read:  Request to get cache line in exclusive (E or M) state.*/\
                                                                        /*        Flag = F_LOCKED: GetSX sets the LOCK, GetX removes the LOCK  */\
                                                                        /*        Flag = F_LLSC: GetSX sets the load-link state, GetX becomes a conditonal write */\
    X(Write,            WriteResp,      Request,    Request,        1, 0,   Cache)   /* Write: Request to write a cache line (does not return the line) */\
    X(FlushLine,        FlushLineResp,  Request,    Request,        1, 0,   Cache)   /* Request to flush a cache line */\
    X(FlushLineInv,     FlushLineResp,  Request,    Request,        1, 0,   Cache)   /* Request to flush and invalidate a cache line */\
    X(FlushAll,         FlushAllResp,   Request,    Request,        1, 0,   Cache)   /* Request to flush entire cache - similar to wbinvd */\
    /* Request Responses */\
    X(GetSResp,         NULLCMD,        Response,   Data,           0, 0,   Cache)   /* Response to a GetS request */\
    X(GetXResp,         NULLCMD,        Response,   Data,           0, 0,   Cache)   /* Response to a GetX request */\
    X(WriteResp,        NULLCMD,        Response,   Ack,            0, 0,   Cache)   /* Response to a Write request*/\
    X(FlushLineResp,    NULLCMD,        Response,   Ack,            0, 0,   Cache)   /* Response to FlushLine request */\
    X(FlushAllResp,     NULLCMD,        Response,   Ack,            0, 0,   Cache)   /* Response to FlushAll request */\
    /* Writebacks, these commands also serve as invalidation acknowledgments */\
    X(PutS,             AckPut,         Request,    Request,        1, 1,   Cache)   /* Clean replacement from S->I:      Remove sharer */\
    X(PutM,             AckPut,         Request,    Request,        1, 1,   Cache)   /* Dirty replacement from M/O->I:    Remove owner and writeback data */\
    X(PutE,             AckPut,         Request,    Request,        1, 1,   Cache)   /* Clean replacement from E->I:      Remove owner but don't writeback data */\
    X(PutX,             AckPut,         Request,    Request,        1, 1,   Cache)   /* Clean downgrade from E->S:        Remove owner, add as sharer, don't writeback data */\
    /* Invalidations*/\
    X(Inv,              AckInv,         Request,    ForwardRequest, 0, 0,   Cache)   /* Other write request:  Invalidate cache line */\
    X(ForceInv,         AckInv,         Request,    ForwardRequest, 0, 0,   Cache)   /* Force invalidation even if line is modified */ \
    /* Invalidates - sent by directory controller */\
    X(Fetch,            FetchResp,      Request,    ForwardRequest, 0, 0,   Cache)   /* Other read request to sharer:  Get data but don't invalidate cache line */\
    X(FetchInv,         FetchResp,      Request,    ForwardRequest, 0, 0,   Cache)   /* Other write request to owner:  Invalidate cache line */\
    X(FetchInvX,        FetchXResp,     Request,    ForwardRequest, 0, 0,   Cache)   /* Other read request to owner:   Downgrade cache line to O/S (Remove exclusivity) */\
    X(FetchResp,        NULLCMD,        Response,   Data,           1, 0,   Cache)   /* response to a Fetch, FetchInv or FetchInvX request */\
    X(FetchXResp,       NULLCMD,        Response,   Data,           1, 0,   Cache)   /* response to a FetchInvX request - indicates a shared copy of the line was kept */\
    /* Others */\
    X(NACK,             NULLCMD,        Response,   Ack,            1, 0,   Cache)   /* NACK response to a message */\
    X(AckInv,           NULLCMD,        Response,   Ack,            1, 0,   Cache)   /* Acknowledgement response to an invalidation request */\
    X(AckPut,           NULLCMD,        Response,   Ack,            0, 0,   Cache)   /* Acknowledgement response to a replacement (Put*) request */\
    X(Put,              AckMove,        Request,    Request,        1, 0,   Move) \
    X(Get,              AckMove,        Request,    Request,        1, 0,   Move) \
    X(AckMove,          NULLCMD,        Response,   Ack,            0, 0,   Move) \
    /* Custom request types. Use to extend memH events by creating a memEventBase-derivative with \
     * memEventBase.cmd set to one of these and custom commands defined in the derivative. See memEventInit (and its derivatives) for an example\
     * Note there is one req for each typical traffic class (request, data, ack) - memH may in the future sort network traffic by class\
     */\
    X(CustomReq,        CustomResp,     Request,    Request,        1, 0,   Custom) \
    X(CustomResp,       NULLCMD,        Response,   Data,           0, 0,   Custom) \
    X(CustomAck,        NULLCMD,        Response,   Ack,            0, 0,   Custom) \
    X(Evict,            NULLCMD,        Request,    Request,        0, 0,   Cache)

/** Valid commands for the MemEvent */
enum class Command {
#define X(a,b,c,d,e,f,g) a,
    X_CMDS
#undef X
    LAST_CMD
};

/** Response commands for MemEvents */
static const Command CommandResponse[] = {
#define X(a,b,c,d,e,f,g) Command::b,
    X_CMDS
#undef X
};

/** Get basic command class (request or response) */
static const BasicCommandClass BasicCommandClassArr[] = {
#define X(a,b,c,d,e,f,g) BasicCommandClass::c,
    X_CMDS
#undef X
};

/** Get complete command type (defined in util.h) */
static const CommandClass CommandClassArr[] = {
#define X(a,b,c,d,e,f,g) CommandClass::d,
    X_CMDS
#undef X
};

static const bool CommandRouteByAddress[] = {
#define X(a,b,c,d,e,f,g) e,
    X_CMDS
#undef X
};

static const bool CommandWriteback[] = {
#define X(a,b,c,d,e,f,g) f,
    X_CMDS
#undef X
};

/** Array of the stringify'd version of the MemEvent Commands.  Useful for printing. */
static const char* CommandString[] __attribute__((unused)) = {
#define X(a,b,c,d,e,f,g) #a ,
    X_CMDS
#undef X
};

static const MemEventType MemEventTypeArr[] = {
#define X(a,b,c,d,e,f,g) MemEventType::g,
    X_CMDS
#undef X
};

// statistics for the network memory inspector
static const std::vector<ElementInfoStatistic> networkMemoryInspector_statistics = {
#define X(a,b,c,d,e,f,g) { #a, #a, "memEvents", 1},
    X_CMDS
#undef X
};


#undef X_CMDS


/******************************************************************************************
 * Coherence states. Not all protocols use all states
 *****************************************************************************************/
/* State NextState */
#define STATE_TYPES \
    X(NP,       NP) /* Invalid/Not present */\
    X(I,        I)  /* Invalid */\
    X(S,        S)  /* Shared */\
    X(E,        E)  /* Exclusive, clean */\
    X(O,        O)  /* Owned, dirty */\
    X(M,        M)  /* Exclusive, dirty */\
    X(IS,       S)  /* Invalid, have issued read request */\
    X(IM,       M)  /* Invalid, have issued write request */\
    X(SM,       M)  /* Shared, have issued upgrade request */\
    X(OM,       M)  /* Owned, have issued upgrade request */\
    X(I_d,      I)  /* I, waiting for dir entry from memory */\
    X(S_d,      S)  /* S, waiting for dir entry from memory */\
    X(M_d,      M)  /* M, waiting for dir entry from memory */\
    X(M_Inv,    M)  /* M, waiting for FetchResp from owner */\
    X(M_InvX,   M)  /* M, waiting for FetchXResp from owner */\
    X(E_Inv,    E)  /* E, waiting for FetchResp from owner */\
    X(E_InvX,   E)  /* E, waiting for FetchXResp from owner */\
    X(S_D,      S)  /* S, waiting for data from memory for another GetS request */\
    X(E_D,      E)  /* E with sharers, waiting for data from memory for another GetS request */\
    X(M_D,      M)  /* M with sharers, waiting for data from memory for another GetS request */\
    X(SM_D,     SM) /* SM, waiting for data from memory for another GetS request */\
    X(SB_D,     S_B) /* S_B, waiting for data from memory for another GetS request */\
    X(S_Inv,    S)  /* S, waiting for Invalidation acks from sharers */\
    X(SM_Inv,   SM) /* SM, waiting for Invalidation acks from sharers */\
    X(SD_Inv,   IS) /* S_D, got Invalidation, waiting for acks */\
    X(MI,       I) \
    X(EI,       I) \
    X(SI,       I) \
    X(M_B,      M)  /* M, blocked while waiting for a response (currently used for flushes) */\
    X(E_B,      E)  /* E, blocked while waiting for a response (currently used for flushes) */\
    X(S_B,      S)  /* S, blocked while waiting for a response (currently used for flushes) */\
    X(I_B,      I)  /* I, blocked while waiting for a response (currently used for flushes) */\
    X(SB_Inv,   S_B)/* Was in S_B, got an Inv, resolving Inv first */\
    X(IA,       I)  /* Still I, but reserve line for a pending fill */\
    X(SA,       S)  /* Still S, but reserve line for a pending fill */\
    X(EA,       E)  /* Still E, but reserve line for a pending fill */\
    X(MA,       M)  /* Still M, but reserve line for a pending fill */\
    X(NULLST,   NULLST)

typedef enum {
#define X(a,b) a,
    STATE_TYPES
#undef X
    LAST_STATE
} State;

/** Array of the stringify'd version of the MemEvent Commands.  Useful for printing. */
static const char* StateString[] __attribute__((unused)) = {
#define X(a,b) #a ,
    STATE_TYPES
#undef X
};

static State NextState[] __attribute__((unused)) = {
#define X(a,b) b,
    STATE_TYPES
#undef X
};

#undef STATE_TYPES

static const std::string NONE = "None";

// Define status types used internally to classify event handling resutls
enum class MemEventStatus { OK, Stall, Reject };

/* Define an address region by start/end & interleaving */
class MemRegion : public SST::Core::Serialization::serializable {
public:
    SST::MemHierarchy::Addr start;             // First address that is part of the region
    SST::MemHierarchy::Addr end;               // Last address that is part of the region
    SST::MemHierarchy::Addr interleaveSize;    // Size of each interleaved chunk
    SST::MemHierarchy::Addr interleaveStep;    // Distance between the start of each interleaved chunk
    static const SST::MemHierarchy::Addr REGION_MAX = std::numeric_limits<SST::MemHierarchy::Addr>::max();

    void setDefault() {
        start = 0;
        interleaveSize = 0;
        interleaveStep = 0;
        end = REGION_MAX;
    }

    void setEmpty() {
        start = 0;
        interleaveSize = 0;
        interleaveStep = 0;
        end = 0;
    }

    bool contains(uint64_t addr) const {
        if (addr >= start && addr <= end) {
            if (interleaveSize == 0) return true;
            SST::MemHierarchy::Addr offset = (addr - start) % interleaveStep;
            return (offset < interleaveSize);
        }
        return false;
    }

    // TODO clean this up to something more succinct
    // We need to compute the set intersection of this MemRegion with MemRegion 'o'
    // The intersection may not be describable as a single MemRegion, so a set is returned
    std::set<MemRegion> intersect(const MemRegion &o) const {
        std::set<MemRegion> regions;
        // Easy case, regions don't overlap
        if (o.end < start || end < o.start)
            return regions; // Empty

        // Easy case, they're equal
        if (*this == o) {
            regions.insert(*this);
            return regions;
        }

        // Easy case, no interleaving
        if (interleaveSize == 0 && o.interleaveSize == 0) {
            MemRegion reg;
            reg.start = std::max(start, o.start);
            reg.end = std::min(end, o.end);
            reg.interleaveSize = 0;
            reg.interleaveStep = 0;
            regions.insert(reg);
            return regions;
        }
       
        // One is interleaved, other is not
        if (interleaveSize == 0) {
            MemRegion reg;
            reg.end = std::min(end, o.end);
            if (start > o.start) {
                uint64_t tmp = start - o.start;
                tmp = tmp / o.interleaveStep;
                tmp = (tmp + 1) * o.interleaveStep + o.start;
                if (tmp > reg.end) return regions;
                reg.start = tmp;
            } else {
                reg.start = o.start;
            }
            if ((reg.end - reg.start) <= o.interleaveStep) {
                reg.interleaveSize = 0;
                reg.interleaveStep = 0;
                reg.end = std::min(reg.end, reg.start + o.interleaveSize);
            } else {
                reg.interleaveSize = o.interleaveSize;
                reg.interleaveStep = o.interleaveStep;
            }
            regions.insert(reg);
            return regions;
        } else if (o.interleaveSize == 0) {
            MemRegion reg;
            reg.end = std::min(end, o.end);
            if (o.start > start) {
                uint64_t tmp = o.start - start;
                tmp = tmp / interleaveStep;
                tmp = (tmp + 1) * interleaveStep + start;
                if (tmp > reg.end) return regions;
                reg.start = tmp;
            } else {
                reg.start = start;
            }
            if ((reg.end - reg.start) <= interleaveStep) {
                reg.interleaveSize = 0;
                reg.interleaveStep = 0;
                reg.end = std::min(reg.end, reg.start + interleaveSize);
            } else {
                reg.interleaveSize = interleaveSize;
                reg.interleaveStep = interleaveStep;
            }
            regions.insert(reg);
            return regions;
        }

        // Check interval from max(start, o.start) to lcm + max(start, o.start)
        // for overlap
        // If overlap, add a region with (start_overlap, min(end, o.end), 1, lcm)
        //  Consecutive regions can be merged
        uint64_t lcm = std::lcm(interleaveStep, o.interleaveStep); 
        uint64_t check_start = std::max(start, o.start);
        uint64_t check_end = check_start + lcm;
        uint64_t region_start = check_start;
        uint64_t region_size = 0;
        for (uint64_t i = check_start; i < check_end; i++) {
            bool in_region = false;
            in_region = (*this).contains(i) && o.contains(i);
            if (in_region) {
                if (region_size == 0)
                    region_start = i;
                region_size++;
            } else if (region_size != 0) {
                MemRegion reg;
                reg.start = region_start;
                reg.end = std::min(end, o.end);
                reg.interleaveSize = region_size;
                reg.interleaveStep = lcm;
                regions.insert(reg);
                region_size = 0;
            }
        }

        if (region_size != 0) {
            MemRegion reg;
            reg.start = region_start;
            reg.end = std::min(end, o.end);
            reg.interleaveSize = region_size;
            reg.interleaveStep = lcm;
            regions.insert(reg);
        }
        return regions;
    }

    bool doesIntersect(const MemRegion &o) const {
        // Easy case, regions don't overlap
        if (o.end < start || end < o.start)
            return false;

        // Easy case, they're equal
        if (*this == o) {
            return true;
        }

        // No interleaving
        if (interleaveStep == 0 && o.interleaveStep == 0) {
            return true;
        }

        // Check interval from max(start, o.start) to lcm + max(start, o.start)
        // for overlap
        uint64_t lcm = std::lcm(interleaveStep, o.interleaveStep); 
        uint64_t check_start = std::max(start, o.start);
        uint64_t check_end = check_start + lcm;
        for (uint64_t i = check_start; i < check_end; i++) {
            if ( (*this).contains(i) && o.contains(i) )
                return true;
        }
        return false;
    }

    /* The one whose range is < the other is <; otherwise the one that has few addresses is < */
    bool operator<(const MemRegion &o) const {
        if (start != o.start)
            return (start < o.start);
        if (end != o.end)
            return (end < o.end);
        return (interleaveSize * o.interleaveStep) < (interleaveStep * o.interleaveSize);
    }

    bool operator==(const MemRegion &o) const {
        return (start == o.start && end == o.end && interleaveSize == o.interleaveSize && interleaveStep == o.interleaveStep);
    }

    bool operator!=(const MemRegion &o) const {
        return !(*this == o);
    }

    std::string toString() const {
        std::ostringstream str;
        str << showbase << hex;
        str << "Start: " << start << " End: " << end;
        str << noshowbase << dec;
        str << " InterleaveSize: " << interleaveSize;
        str << " InterleaveStep: " << interleaveStep;
        return str.str();
    }

    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        ser & start;
        ser & end;
        ser & interleaveSize;
        ser & interleaveStep;
    }
private:
    ImplementSerializable(SST::MemHierarchy::MemRegion)
};

} /* End namespace MemHierarchy */
} /* End namespace SST */
#endif
