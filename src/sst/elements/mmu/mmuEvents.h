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

#ifndef MMU_EVENTS_H
#define MMU_EVENTS_H

#include <cstddef>
#include <cstdint>
#include <sst/core/event.h>
#include "mmuTypes.h"

namespace SST {

namespace MMU_Lib {

class TlbInitEvent  : public SST::Event {
  public:

    TlbInitEvent() : Event(), pageShift(0) {}

    TlbInitEvent( int pageShift ) : Event(), pageShift(pageShift) { }
    virtual ~TlbInitEvent() {}
    int getPageShift() { return pageShift; }

  private:

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(pageShift);
    }

    ImplementSerializable(TlbInitEvent);

    int pageShift;
};

class TlbMissEvent  : public SST::Event {
  public:

    TlbMissEvent() : Event() {}
    TlbMissEvent( RequestID id, int hwThread, size_t vpn, uint32_t perms, uint64_t instPtr, uint64_t memAddr  )
        : Event(), id(id), hwThread(hwThread), vpn(vpn), perms(perms), instPtr(instPtr), memAddr(memAddr) { }
    virtual ~TlbMissEvent() {}

    RequestID getReqId() { return id; }
    int getHardwareThread() { return hwThread; }
    size_t getVPN() { return vpn; }
    uint32_t getPerms() { return perms; }
    uint64_t getInstPtr() { return instPtr; }
    uint64_t getMemAddr() { return memAddr; }

  private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(id);
        SST_SER(hwThread);
        SST_SER(vpn);
        SST_SER(perms);
        SST_SER(instPtr);
        SST_SER(memAddr);
    }
        ImplementSerializable(TlbMissEvent);

        RequestID id;
        int hwThread;
        size_t vpn;
        uint32_t perms;
        uint64_t instPtr;
        uint64_t memAddr;

    };

    class TlbFillEvent  : public SST::Event {

      public:
        TlbFillEvent() : Event() {}
        TlbFillEvent( RequestID id, PTE pte ) : Event(), id(id), perms(pte.perms), ppn(pte.ppn), success(true) { }
        TlbFillEvent( RequestID id ) : Event(), id(id), success(false) { }
        virtual ~TlbFillEvent() {}


    RequestID getReqId() { return id; }
    size_t getPPN() { return ppn; }
    int32_t getPerms() { return perms; }
    bool isSuccess() { return success; }

  private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(id);
        SST_SER(perms);
        SST_SER(ppn);
        SST_SER(success);
    }
    ImplementSerializable(TlbFillEvent);

    RequestID id;
    uint32_t ppn;
    uint32_t perms;
    bool success;

};

class TlbFlushReqEvent  : public SST::Event {

  public:
    TlbFlushReqEvent() : Event() {}
    TlbFlushReqEvent( unsigned hwThread ) : Event(), hwThread(hwThread) { }
    virtual ~TlbFlushReqEvent() {}

    unsigned getHwThread() { return hwThread; }

  private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(hwThread);
    }
    ImplementSerializable(TlbFlushReqEvent);

    unsigned hwThread;
};

class TlbFlushRespEvent  : public SST::Event {


  public:
    TlbFlushRespEvent() : Event() {}
    TlbFlushRespEvent( unsigned hwThread ) : Event(), hwThread(hwThread) { }
    virtual ~TlbFlushRespEvent() {}

    unsigned getHwThread() { return hwThread; }

  private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(hwThread);
    }
    ImplementSerializable(TlbFlushRespEvent);

    unsigned hwThread;
};

} //namespace MMU_Lib
} //namespace SST

#endif /* MMU_EVENTS_H */
