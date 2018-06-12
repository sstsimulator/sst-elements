// Copyright 2016-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2016-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _ARIELALLOCTRACKEV_H
#define _ARIELALLOCTRACKEV_H

namespace SST {
namespace ArielComponent {

class arielAllocTrackEvent : public SST::Event 
{
    public:
        enum arielAllocTrackType {
            ALLOC,
            FREE,
            BUOY
        };

        arielAllocTrackEvent(arielAllocTrackType t, uint64_t va, uint64_t len, uint32_t lev, uint64_t ip) :
            SST::Event(), type(t), virtualAddress(va), allocateLength(len), level(lev), instPtr(ip) { }

        arielAllocTrackType getType() const {return type;}
        uint64_t getVirtualAddress() const {return virtualAddress;}
        uint64_t getAllocateLength() const {return allocateLength;}
        uint32_t getLevel() const {return level;}
        uint64_t getInstructionPointer() const {return instPtr;}

    private:
        arielAllocTrackType type;
        uint64_t virtualAddress;
        uint64_t allocateLength;
        uint32_t level;
        uint64_t instPtr;

        arielAllocTrackEvent() {} // For serialization only

    public:
        void serialize_order(SST::Core::Serialization::serializer &ser)  override {
            Event::serialize_order(ser);
            ser & type;
            ser & virtualAddress;
            ser & allocateLength;
            ser & level;
        }

        ImplementSerializable(SST::ArielComponent::arielAllocTrackEvent);
};

} // namespace ArielComponent
} // namespace SST

#endif /* ARIELALLOCTRACKEV_H */
