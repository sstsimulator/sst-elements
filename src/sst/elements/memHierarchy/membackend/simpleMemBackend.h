// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_SIMPLE_MEM_BACKEND
#define _H_SST_MEMH_SIMPLE_MEM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class SimpleMemory : public SimpleMemBackend {
public:
    SimpleMemory();
    SimpleMemory(Component *comp, Params &params);
    bool issueRequest(ReqId, Addr, bool, unsigned );
    virtual int32_t getMaxReqPerCycle() { return 1; }
    
public:
    class MemCtrlEvent : public SST::Event {
    public:
        MemCtrlEvent( ReqId id_) : SST::Event(), reqId(id_)
        { }

		ReqId reqId;
     
    private:   
        MemCtrlEvent() {} // For Serialization only
        
    public:
        void serialize_order(SST::Core::Serialization::serializer &ser)  override {
            Event::serialize_order(ser);
            ser & reqId;  // Cannot serialize pointers unless they are a serializable object
       }
        
        ImplementSerializable(SST::MemHierarchy::SimpleMemory::MemCtrlEvent);
    };

    void handleSelfEvent(SST::Event *event);

    Link *self_link;
};

}
}

#endif
