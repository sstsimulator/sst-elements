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

#ifndef MEMHIERARCHY_MEMTYPES_H
#define MEMHIERARCHY_MEMTYPES_H

#include <sst/core/sst_types.h>

#include <sst/core/elibase.h>   // For ElementInfoStatistic

#include "sst/elements/memHierarchy/util.h"

namespace SST { namespace MemHierarchy {

using namespace std;


// Command attributes
enum class CommandClass { Request, Data, Ack, ForwardRequest };     // TODO - route messages on VCs based on command class
enum class BasicCommandClass {Request, Response};                   // Whether a command is a request or response
enum class MemEventType { Cache, Move, Custom };                    // For parsing which kind of event a MemEventBase is, 'Custom' is a catchall for types memH doesn't know about



/******************************************************************************************
 *  Commands used throughout MemH
 *  Not all components handle all types
 *
 *  Command, ResponseCmd, BasicCommandClass, CommandClass, cpuSideRequest, Writeback, EventType
 *****************************************************************************************/
#define X_CMDS \
    X(NULLCMD,          NULLCMD,        Request,    Request,        1, 0,   Cache)   /* Dummy command */\
    /* Requests */ \
    X(GetS,             GetSResp,       Request,    Request,        1, 0,   Cache)   /* Read:  Request to get cache line in S state */\
    X(GetX,             GetXResp,       Request,    Request,        1, 0,   Cache)   /* Write: Request to get cache line in M state */\
    X(GetSX,            GetSResp,       Request,    Request,        1, 0,   Cache)   /* Read:  Request to get cache line in M state with a LOCK flag. Invalidates will block until LOCK flag is lifted */\
                                                                        /*        GetSX sets the LOCK, GetX removes the LOCK  */\
    X(FlushLine,        FlushLineResp,  Request,    Request,        1, 0,   Cache)   /* Request to flush a cache line */\
    X(FlushLineInv,     FlushLineResp,  Request,    Request,        1, 0,   Cache)   /* Request to flush and invalidate a cache line */\
    X(FlushAll,         FlushAllResp,   Request,    Request,        1, 0,   Cache)   /* Request to flush entire cache - similar to wbinvd */\
    /* Request Responses */\
    X(GetSResp,         NULLCMD,        Response,   Data,           0, 0,   Cache)   /* Response to a GetS request */\
    X(GetXResp,         NULLCMD,        Response,   Data,           0, 0,   Cache)   /* Response to a GetX request */\
    X(FlushLineResp,    NULLCMD,        Response,   Ack,            0, 0,   Cache)   /* Response to FlushLine request */\
    X(FlushAllResp,     NULLCMD,        Response,   Ack,            0, 0,   Cache)   /* Response to FlushAll request */\
    /* Writebacks, these commands also serve as invalidation acknowledgments */\
    X(PutS,             AckPut,         Request,    Request,        1, 1,   Cache)   /* Clean replacement from S->I:      Remove sharer */\
    X(PutM,             AckPut,         Request,    Request,        1, 1,   Cache)   /* Dirty replacement from M/O->I:    Remove owner and writeback data */\
    X(PutE,             AckPut,         Request,    Request,        1, 1,   Cache)   /* Clean replacement from E->I:      Remove owner but don't writeback data */\
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
    X(CustomAck,        NULLCMD,        Response,   Ack,            0, 0,   Custom)

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

static const bool CommandCPUSide[] = {
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
    X(S_Inv,    S)  /* S, waiting for Invalidation acks from sharers */\
    X(SM_Inv,   SM) /* SM, waiting for Invalidation acks from sharers */\
    X(SD_Inv,   IS) /* S_D, got Invalidation, waiting for acks */\
    X(MI,       I) \
    X(EI,       I) \
    X(SI,       I) \
    X(S_B,      S)  /* S, blocked while waiting for a response (currently used for flushes) */\
    X(I_B,      I)  /* I, blocked while waiting for a response (currently used for flushes) */\
    X(SB_Inv,   S_B)/* Was in S_B, got an Inv, resolving Inv first */\
    X(NULLST,   NULLST)

typedef enum {
#define X(a,b) a,
    STATE_TYPES
#undef X
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

}}


/* Define an address region by start/end & interleaving */
struct MemRegion {
    uint64_t start;
    uint64_t end;
    uint64_t interleaveSize;
    uint64_t interleaveStep;

    void setDefault() {
        start = interleaveSize = interleaveStep = 0;
        end = (uint64_t) - 1;
    }

    bool contains(uint64_t addr) const {
        if (addr >= start && addr < end) {
            if (interleaveSize == 0) return true;
            uint64_t offset = (addr - start) % interleaveStep;
            return (offset < interleaveSize);
        }
        return false;
    }

    bool operator<(const MemRegion &o) const {
        return (start < o.start);
    }

    bool operator==(const MemRegion &o) const {
        return (start == o.start && end == o.end);
    }

    bool operator!=(const MemRegion &o) const {
        return !(*this == o);
    }

    std::string toString() const {
        std::ostringstream str;
        str << "Start: " << start << " End: " << end;
        str << " InterleaveSize: " << interleaveSize;
        str << " InterleaveStep: " << interleaveStep;
        return str.str();
    }
};


#endif
