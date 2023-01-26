// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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

#include <sst/core/event.h>
#include "mmuTypes.h"

namespace SST {

namespace MMU_Lib {

class TlbInitEvent  : public SST::Event {
  public:

    TlbInitEvent( int pageShift ) : Event(), pageShift(pageShift) { }
    int getPageShift() { return pageShift; }
  private:

    int pageShift;
    NotSerializable(TlbInitEvent)
};

class TlbMissEvent  : public SST::Event {
  public:

    TlbMissEvent( RequestID id, int hwThread, size_t vpn, uint32_t perms, uint64_t instPtr, uint64_t memAddr  )
        : Event(), id(id), hwThread(hwThread), vpn(vpn), perms(perms), instPtr(instPtr), memAddr(memAddr) { }
    RequestID getReqId() { return id; }
    int getHardwareThread() { return hwThread; }
    size_t getVPN() { return vpn; }
    uint32_t getPerms() { return perms; }
    uint64_t getInstPtr() { return instPtr; }
    uint64_t getMemAddr() { return memAddr; }
  private:
    RequestID id;
    int hwThread;
    size_t vpn;
    uint32_t perms;
    uint64_t instPtr;
    uint64_t memAddr;

    NotSerializable(TlbMissEvent)
};

class TlbFillEvent  : public SST::Event {
  public:

    TlbFillEvent( RequestID id, PTE pte ) : Event(), id(id), pte(pte), success(true) { }
    TlbFillEvent( RequestID id ) : Event(), id(id), success(false) { }
    RequestID getReqId() { return id; }
    size_t getPPN() { return pte.ppn; }
    int32_t getPerms() { return pte.perms; }
    bool isSuccess() { return success; }
  private:
    RequestID id;
    PTE    pte;
    bool success;

    NotSerializable(TlbFillEvent)
};

class TlbFlushEvent  : public SST::Event {
  public:

    TlbFlushEvent( unsigned hwThread ) : Event(), hwThread(hwThread) { }
    unsigned getHwThread() { return hwThread; }
  private:
    unsigned hwThread;

    NotSerializable(TlbFillEvent)
};

} //namespace MMU_Lib
} //namespace SST

#endif /* MMU_EVENTS_H */
