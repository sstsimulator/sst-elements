// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


//
#ifndef _H_SST_MESSIER_MEM_EVENT
#define _H_SST_MESSIER_MEM_EVENT

#include <cstdint>
#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/core/link.h>
#include <sst/core/event.h>

namespace SST {
namespace MessierComponent {

typedef uint64_t Addr;
typedef uint64_t ReqId;

class MemReqEvent : public SST::Event {
  public:
    MemReqEvent(ReqId id, Addr addr1, bool isWrite, unsigned numBytes, uint32_t flags) :
        SST::Event(), reqId(id), addr(addr1), isWrite(isWrite), numBytes(numBytes), flags(flags)
    {

        eventID  = generateUniqueId();
    }

    ReqId getReqId() { return reqId; }
    Addr getAddr() { return addr; }
    bool getIsWrite() { return isWrite; }
    unsigned  getNumBytes() { return numBytes; }
    uint32_t getFlags() { return flags; }
    id_type getID(void) const { return eventID; }

  private:
    MemReqEvent() {} // For Serialization only

    ReqId reqId;
    Addr addr;
    bool isWrite;
    unsigned numBytes;
    uint32_t flags;
    id_type eventID;

  public:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        Event::serialize_order(ser);
        SST_SER(reqId);
        SST_SER(addr);
        SST_SER(isWrite);
        SST_SER(numBytes);
        SST_SER(flags);
        SST_SER(eventID);
    }

    ImplementSerializable(MemReqEvent);
};

class MemRespEvent : public SST::Event {
  public:
    MemRespEvent(ReqId id, Addr addr, uint32_t flags) :
        SST::Event(), reqId(id), addr(addr), flags(flags)
    {
        eventID  = generateUniqueId();
    }

    ReqId getReqId() { return reqId; }
    Addr getAddr() { return addr; }
    uint32_t getFlags() { return flags; }
    id_type getID(void) const { return eventID; }

  private:
    MemRespEvent() {} // For Serialization only

    ReqId reqId;
    Addr addr;
    uint32_t flags;
    id_type eventID;

  public:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        Event::serialize_order(ser);
        SST_SER(reqId);
        SST_SER(flags);
        SST_SER(addr);
        SST_SER(eventID);
    }

    ImplementSerializable(MemRespEvent);
};
}
}

#endif
