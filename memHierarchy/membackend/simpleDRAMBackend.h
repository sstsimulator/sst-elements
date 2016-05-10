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


#ifndef _H_SST_MEMH_SIMPLE_DRAM_BACKEND
#define _H_SST_MEMH_SIMPLE_DRAM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class SimpleDRAM : public MemBackend {
public:
    SimpleDRAM();
    SimpleDRAM(Component *comp, Params &params);
    bool issueRequest(DRAMReq *req);
    
    typedef enum {OPEN, CLOSED, DYNAMIC, TIMEOUT } RowPolicy;

private:
    void handleSelfEvent(SST::Event *event);

    Link *self_link;
    Output * output;

    int * openRow;
    bool * busy;
   
    // Mapping parameters
    unsigned int lineOffset;
    unsigned int rowOffset;
    unsigned int bankMask;

    // Time parameters
    unsigned int tCAS;
    unsigned int tRCD;
    unsigned int tRP;

    RowPolicy policy;

    Statistic<uint64_t> * statRowHit;
    Statistic<uint64_t> * statRowMissNoRP;
    Statistic<uint64_t> * statRowMissRP;

public:
    class MemCtrlEvent : public SST::Event {
    public:
        MemCtrlEvent(DRAMReq* req, int bank) : SST::Event(), req(req), bank(bank) { }
        
        DRAMReq *req;
        int bank;
    private:
        MemCtrlEvent() {} // For Serialization only
    
    public:
        void serialize_order(SST::Core::Serialization::serializer &ser) {
            Event::serialize_order(ser);
            ser & req;  // Cannot serialize pointers unless they are a serializable object
            ser & bank;
        }
        ImplementSerializable(SST::MemHierarchy::SimpleDRAM::MemCtrlEvent);
    };


};

}
}

#endif
