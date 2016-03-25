// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_SIMPLE_MEM_BACKEND
#define _H_SST_MEMH_SIMPLE_MEM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class SimpleMemory : public MemBackend {
public:
    SimpleMemory();
    SimpleMemory(Component *comp, Params &params);
    bool issueRequest(DRAMReq *req);
    
public:
    class MemCtrlEvent : public SST::Event {
    public:
        MemCtrlEvent(DRAMReq* req) : SST::Event(), req(req)
        { }

        DRAMReq *req;
     
    private:   
        MemCtrlEvent() {} // For Serialization only
        
    public:
        void serialize_order(SST::Core::Serialization::serializer &ser) {
            Event::serialize_order(ser);
//            ser & req;  // Cannot serialize pointers unless they are a serializable object
       }
        
        ImplementSerializable(SST::MemHierarchy::SimpleMemory::MemCtrlEvent);
    };

    void handleSelfEvent(SST::Event *event);

    Link *self_link;
};

}
}

#endif
