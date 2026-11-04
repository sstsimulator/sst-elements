// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
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

#include <utility>
#include <vector>

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
 * The event owns the CustomData object and deletes it when the event is deleted.
 * Exactly one event owns a given CustomData object at a time; copies of the
 * event (copy constructor, clone()) receive a deep copy of the data.
 */
class CustomMemEvent : public MemEventBase {
public:

    /** Creates a new CustomMemEvent. The event takes ownership of data. */
    CustomMemEvent(std::string src, Command cmd, Interfaces::StandardMem::CustomData* data) : MemEventBase(src, cmd), data_(data) {}

    /** Copy constructor. The copy receives a deep copy of the CustomData object. */
    CustomMemEvent(const CustomMemEvent& other) : MemEventBase(other), data_(copyCustomData(other.data_)) {}

    CustomMemEvent& operator=(const CustomMemEvent&) = delete;

    virtual ~CustomMemEvent() { delete data_; }

    /**
     * Create a response to this event.
     * Ownership of this event's CustomData object is transferred to the response
     * so that any modifications made while handling the request are returned.
     * This event no longer holds data afterwards and is expected to be deleted.
     */
    CustomMemEvent* makeResponse() override {
        CustomMemEvent *me = new CustomMemEvent(*this, releaseCustomData());
        me->setResponse(this);
        return me;
    }

    /** Return size of the event - for calculating bandwidth used */
    virtual uint32_t getEventSize() override { return data_ ? data_->getSize() : 0; }

    /** Get verbose print of the event */
    virtual std::string getVerboseString(int level = 1) override {
        return MemEventBase::getVerboseString(level) + "Data: " + (data_ ? data_->getString() : "(none)");
    }

    /** Get brief print of the event */
    virtual std::string getBriefString() override {
        return MemEventBase::getBriefString() + "Data: " + (data_ ? data_->getString() : "(none)");
    }

    /** Returns address that determines where this event is sent to */
    virtual Addr getRoutingAddress() override {
        return data_ ? data_->getRoutingAddress() : 0;
    }

    virtual size_t getPayloadSize() override {
        return data_ ? data_->getSize() : 0;
    }

    /** Return a copy of this event, including a deep copy of the CustomData object */
    virtual MemEventBase* clone(void) override {
        return new CustomMemEvent(*this);
    }

    /** Get the CustomData object. Ownership is retained by this event. */
    virtual Interfaces::StandardMem::CustomData* getCustomData() { return data_; }

    /** Set the CustomData object. This event takes ownership of data and deletes the previous object. */
    virtual void setCustomData(Interfaces::StandardMem::CustomData* data) {
        if (data != data_)
            delete data_;
        data_ = data;
    }

    /** Release the CustomData object. Ownership is transferred to the caller and this event no longer holds data. */
    virtual Interfaces::StandardMem::CustomData* releaseCustomData() { return std::exchange(data_, nullptr); }

protected:
    Interfaces::StandardMem::CustomData* data_ = nullptr;

    CustomMemEvent() {} // For serialization only

    /* Copy the event fields from other but take ownership of data instead of copying other's data */
    CustomMemEvent(const CustomMemEvent& other, Interfaces::StandardMem::CustomData* data) : MemEventBase(other), data_(data) {}

    /* Deep copy a CustomData object via serialization (CustomData objects are required to be serializable) */
    static Interfaces::StandardMem::CustomData* copyCustomData(Interfaces::StandardMem::CustomData* data) {
        if (data == nullptr)
            return nullptr;
        SST::Core::Serialization::serializer ser;
        ser.start_sizing();
        SST_SER(data);
        size_t size = ser.size();

        std::vector<char> buffer(size);
        ser.start_packing(buffer.data(), size);
        SST_SER(data);

        Interfaces::StandardMem::CustomData* copy = nullptr;
        ser.start_unpacking(buffer.data(), size);
        SST_SER(copy);
        return copy;
    }

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        MemEventBase::serialize_order(ser);
        SST_SER(data_);
    }

    ImplementSerializable(SST::MemHierarchy::CustomMemEvent);
};

}}


#endif  /* MEMHIERARHCY_MEMEVENTCUSTOM_H */
