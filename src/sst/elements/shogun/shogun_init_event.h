// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SHOGUN_INIT_EVENT
#define _H_SHOGUN_INIT_EVENT

#include <sst/core/event.h>

namespace SST {
namespace Shogun {

    class ShogunInitEvent : public SST::Event {

    public:
        ShogunInitEvent()
            : port_count(01)
            , netID(-1)
            , queue_slots(-1)
        {
        }

        ShogunInitEvent(
            const int portCnt,
            const int netid,
            const int qSlot)
            : port_count(portCnt)
            , netID(netid)
            , queue_slots(qSlot)
        {
        }

        int getPortCount() const
        {
            return port_count;
        }

        int getNetworkID() const
        {
            return netID;
        }

        int getQueueSlotCount() const
        {
            return queue_slots;
        }

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            Event::serialize_order(ser);
            ser& port_count;
            ser& netID;
            ser& queue_slots;
        }

        ImplementSerializable(SST::Shogun::ShogunInitEvent);

    private:
        int port_count;
        int netID;
        int queue_slots;
    };

}
}

#endif
