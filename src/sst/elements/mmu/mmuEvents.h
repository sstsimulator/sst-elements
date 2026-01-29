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

#include <sst/core/event.h>
#include "mmuTypes.h"

namespace SST {

namespace MMU_Lib {

class TlbInitEvent  : public SST::Event {
  public:

    TlbInitEvent() : Event(), page_shift_(0) {}

    TlbInitEvent( uint32_t page_shift ) : Event(), page_shift_(page_shift) { }
    virtual ~TlbInitEvent() {}
    uint32_t getPageShift() { return page_shift_; }

  private:

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(page_shift_);
    }

    ImplementSerializable(TlbInitEvent);

    uint32_t page_shift_;
};

class TlbMissEvent  : public SST::Event {
  public:

    TlbMissEvent() : Event() {}
    TlbMissEvent( RequestID id, uint32_t hw_thread, uint32_t vpn, uint32_t perms, uint64_t inst_ptr, uint64_t mem_addr  )
        : Event(), id_(id), hw_thread_(hw_thread), vpn_(vpn), perms_(perms), inst_ptr_(inst_ptr), mem_addr_(mem_addr) { }
    virtual ~TlbMissEvent() {}

    RequestID getReqId() { return id_; }
    uint32_t getHardwareThread() { return hw_thread_; }
    uint32_t getVPN() { return vpn_; }
    uint32_t getPerms() { return perms_; }
    uint64_t getInstPtr() { return inst_ptr_; }
    uint64_t getMemAddr() { return mem_addr_; }

  private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(id_);
        SST_SER(hw_thread_);
        SST_SER(vpn_);
        SST_SER(perms_);
        SST_SER(inst_ptr_);
        SST_SER(mem_addr_);
    }
        ImplementSerializable(TlbMissEvent);

        RequestID id_;
        uint32_t hw_thread_;
        uint32_t vpn_;
        uint32_t perms_;
        uint64_t inst_ptr_;
        uint64_t mem_addr_;

    };

    class TlbFillEvent  : public SST::Event {

      public:
        TlbFillEvent() : Event() {}
        TlbFillEvent( RequestID id, PTE pte ) : Event(), id_(id), perms_(pte.perms), ppn_(pte.ppn), success_(true) { }
        TlbFillEvent( RequestID id ) : Event(), id_(id), success_(false) { }
        virtual ~TlbFillEvent() {}


    RequestID getReqId() { return id_; }
    uint32_t getPPN() { return ppn_; }
    uint32_t getPerms() { return perms_; }
    bool isSuccess() { return success_; }

  private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(id_);
        SST_SER(perms_);
        SST_SER(ppn_);
        SST_SER(success_);
    }
    ImplementSerializable(TlbFillEvent);

    RequestID id_;
    uint32_t ppn_;
    uint32_t perms_;
    bool success_;

};

class TlbFlushReqEvent  : public SST::Event {

  public:
    TlbFlushReqEvent() : Event() {}
    TlbFlushReqEvent( uint32_t hw_thread ) : Event(), hw_thread_(hw_thread) { }
    virtual ~TlbFlushReqEvent() {}

    uint32_t getHwThread() { return hw_thread_; }

  private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(hw_thread_);
    }
    ImplementSerializable(TlbFlushReqEvent);

    uint32_t hw_thread_;
};

class TlbFlushRespEvent  : public SST::Event {


  public:
    TlbFlushRespEvent() : Event() {}
    TlbFlushRespEvent( uint32_t hw_thread ) : Event(), hw_thread_(hw_thread) { }
    virtual ~TlbFlushRespEvent() {}

    uint32_t getHwThread() { return hw_thread_; }

  private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(hw_thread_);
    }
    ImplementSerializable(TlbFlushRespEvent);

    uint32_t hw_thread_;
};

} //namespace MMU_Lib
} //namespace SST

#endif /* MMU_EVENTS_H */
