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


#ifndef _H_SST_ARIEL_FLUSH_EVENT
#define _H_SST_ARIEL_FLUSH_EVENT

#include <sst_config.h> 
#include <../Opal/Opal_Event.h>
#include "arielevent.h"
#include "arielcore.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielFlushEvent : public ArielEvent {

    public:
        ArielFlushEvent(uint64_t vAddr, uint64_t cacheLineSize) :
                virtualAddress(vAddr){
                    length = cacheLineSize;
                }
        ~ArielFlushEvent() {}
        
        ArielEventType getEventType() const { return FLUSH; }
        uint64_t getVirtualAddress() const { return virtualAddress;}
        uint64_t getAddress() const { return address; }
        uint64_t getLength() const { return length; }

    protected:
        uint64_t virtualAddress;
        uint64_t address;
        uint64_t length;

};

}
}

#endif
