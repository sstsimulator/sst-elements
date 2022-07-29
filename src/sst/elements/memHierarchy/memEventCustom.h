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

#ifndef MEMHIERARHCY_MEMEVENTCUSTOM_H
#define MEMHIERARHCY_MEMEVENTCUSTOM_H

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/warnmacros.h>
#include <sst/core/interfaces/stdMem.h>

#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/memEventBase.h"

namespace SST { namespace MemHierarchy {

using namespace std;

/**
 * Class encapsulating custom events
 * memHierarchy will route the request to its destination but will not
 * directly handle the event en route (e.g., perform cache lookups, etc.)
 *
 * This event wraps an Interfaces::StandardMem::CustomData object.
 */
class CustomMemEvent : public MemEventBase {
public:

    /** Creates a new CustomMemEvent */
    CustomMemEvent(std::string src, Command cmd, Interfaces::StandardMem::CustomData* data) : MemEventBase(src, cmd), data_(data) {}

    CustomMemEvent* makeResponse() override {
        CustomMemEvent *me = new CustomMemEvent(*this);
        me->setResponse(this);
        return me;
    }

    /** Return size of the event - for calculating bandwidth used */
    virtual uint32_t getEventSize() override { return data_->getSize(); }

    /** Get verbose print of the event */ 
    virtual std::string getVerboseString(int level = 1) override {
        return MemEventBase::getVerboseString(level) + "Data: " + data_->getString();
    }

    /** Get brief print of the event */
    virtual std::string getBriefString() override {
        return MemEventBase::getBriefString() + "Data: " + data_->getString();
    }
    
    /** Returns address that determines where this event is sent to */
    virtual Addr getRoutingAddress() override {
        return data_->getRoutingAddress();
    }

    virtual size_t getPayloadSize() override {
        return data_->getSize();
    }

    virtual MemEventBase* clone(void) override {
        return new CustomMemEvent(*this);
    }

    virtual Interfaces::StandardMem::CustomData* getCustomData() { return data_; }
    virtual void setCustomData(Interfaces::StandardMem::CustomData* data) { data_ = data; }

protected:
    Interfaces::StandardMem::CustomData* data_;
    
    CustomMemEvent() {} // For serialization only

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        MemEventBase::serialize_order(ser);
        ser & data_;
    }

    ImplementSerializable(SST::MemHierarchy::CustomMemEvent);
};

}}


#endif  /* MEMHIERARHCY_MEMEVENTCUSTOM_H */
