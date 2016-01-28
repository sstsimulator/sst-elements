// Copyright 2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2015, Sandia Corporation
// All rights reserved.
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
        FREE
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

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(type);
        ar & BOOST_SERIALIZATION_NVP(virtualAddress);
        ar & BOOST_SERIALIZATION_NVP(allocateLength);
        ar & BOOST_SERIALIZATION_NVP(level);
    }
};

} // namespace ArielComponent
} // namespace SST

#endif /* ARIELALLOCTRACKEV_H */
