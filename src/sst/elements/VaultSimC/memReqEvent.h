// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.    

#ifndef _H_SST_VAULTSIM_MEM_EVENT
#define _H_SST_VAULTSIM_MEM_EVENT

#include <sst/core/event.h>
#include <sst/elements/VaultSimC/VaultSimC.h>

namespace SST {
namespace VaultSim {

typedef uint64_t Addr;
typedef uint64_t ReqId;

class MemReqEvent : public SST::Event {
  public:
    MemReqEvent(ReqId id, Addr addr, bool isWrite, unsigned numBytes) : 
		SST::Event(), reqId(id), addr(addr), isWrite(isWrite), numBytes(numBytes) 
    { }

	ReqId reqId;
	Addr addr;
	bool isWrite;
	unsigned numBytes;

  private:
    MemReqEvent() {} // For Serialization only

  public:
    void serialize_order(SST::Core::Serialization::serializer &ser) {
        Event::serialize_order(ser);
        ser & reqId;  
        ser & addr;
        ser & isWrite;
        ser & numBytes;
    }

    ImplementSerializable(MemReqEvent);
};

class MemRespEvent : public SST::Event {
  public:
    MemRespEvent(ReqId id) : SST::Event(), reqId(id) { }

	ReqId reqId;

  private:
    MemRespEvent() {} // For Serialization only

  public:
    void serialize_order(SST::Core::Serialization::serializer &ser) {
        Event::serialize_order(ser);
        ser & reqId;  
    }

    ImplementSerializable(MemRespEvent);
};
}
}

#endif
