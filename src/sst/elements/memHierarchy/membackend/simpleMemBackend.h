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


#ifndef _H_SST_MEMH_SIMPLE_MEM_BACKEND
#define _H_SST_MEMH_SIMPLE_MEM_BACKEND

#include "sst/elements/memHierarchy/membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class SimpleMemory : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(SimpleMemory, "memHierarchy", "simpleMem", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Basic constant-access-time memory timing model", SST::MemHierarchy::SimpleMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"access_time", "(string) Constant latency of memory operations. With units (SI ok).", "100ns"} )

/* Begin class definition */
    SimpleMemory();
    SimpleMemory(ComponentId_t id, Params &params);
    bool issueRequest(ReqId, Addr, bool, unsigned );
    virtual bool isClocked() { return false; }

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
