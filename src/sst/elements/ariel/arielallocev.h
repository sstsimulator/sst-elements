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


#ifndef _H_SST_ARIEL_ALLOC_EVENT
#define _H_SST_ARIEL_ALLOC_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielAllocateEvent : public ArielEvent {

    public:
        ArielAllocateEvent(uint64_t vAddr, uint64_t len, uint32_t lev, uint64_t ip) :
                virtualAddress(vAddr),
                allocateLength(len),
                level(lev),
                instPtr(ip) {

        }

        ~ArielAllocateEvent() {}
        ArielEventType getEventType() const { return MALLOC; }
        uint64_t getVirtualAddress() const { return virtualAddress; }
        uint64_t getAllocationLength() const { return allocateLength; }
        uint32_t getAllocationLevel() const { return level; }
        uint64_t getInstructionPointer() const { return instPtr; }

    protected:
        uint64_t virtualAddress;
        uint64_t allocateLength;
        uint32_t level;
        uint64_t instPtr;

};

class ArielMmapEvent : public ArielEvent {

    public:
        ArielMmapEvent(uint32_t fileID, uint64_t vAddr, uint64_t len, uint32_t lev, uint64_t ip) :
                FileID(fileID),
                virtualAddress(vAddr),
                allocateLength(len),
                level(lev),
                instPtr(ip) {

        }

        ~ArielMmapEvent() {}
        ArielEventType getEventType() const { return MMAP; }
        uint64_t getVirtualAddress() const { return virtualAddress; }
        uint64_t getAllocationLength() const { return allocateLength; }
        uint32_t getAllocationLevel() const { return level; }
        uint64_t getInstructionPointer() const { return instPtr; }
        uint32_t getFileID() const { return FileID; }

    protected:
        uint64_t virtualAddress;
        uint64_t allocateLength;
        uint32_t level;
        uint64_t instPtr;
        uint32_t FileID;

};


}
}

#endif
