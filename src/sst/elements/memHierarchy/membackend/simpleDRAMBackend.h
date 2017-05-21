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


#ifndef _H_SST_MEMH_SIMPLE_DRAM_BACKEND
#define _H_SST_MEMH_SIMPLE_DRAM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class SimpleDRAM : public SimpleMemBackend {
public:
    SimpleDRAM();
    SimpleDRAM(Component *comp, Params &params);
    bool issueRequest( ReqId, Addr, bool, unsigned );
    
    typedef enum {OPEN, CLOSED, DYNAMIC, TIMEOUT } RowPolicy;

private:
    void handleSelfEvent(SST::Event *event);

    Link *self_link;

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
        MemCtrlEvent(int bank, ReqId reqId ) : SST::Event(), bank(bank), close(false), reqId(reqId) { }
        MemCtrlEvent(int bank ) : SST::Event(), bank(bank), close(true) { }
        
        int bank;
		bool close;
        ReqId reqId;
    private:
        MemCtrlEvent() {} // For Serialization only
    
    public:
        void serialize_order(SST::Core::Serialization::serializer &ser)  override {
            Event::serialize_order(ser);
            ser & reqId;  // Cannot serialize pointers unless they are a serializable object
            ser & bank;
			ser & close;
        }
        ImplementSerializable(SST::MemHierarchy::SimpleDRAM::MemCtrlEvent);
    };


};

}
}

#endif
