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


#ifndef __SST_MEMH_BACKEND__
#define __SST_MEMH_BACKEND__

#include <sst/core/event.h>
#include <sst/core/output.h>

#include <iostream>
#include <map>
#include <vector>

#include "sst/elements/memHierarchy/membackend/memBackendConvertor.h"
#include "sst/elements/memHierarchy/membackend/simpleMemBackendConvertor.h"
#include "sst/elements/memHierarchy/membackend/flagMemBackendConvertor.h"
#include "sst/elements/memHierarchy/membackend/extMemBackendConvertor.h"

#define NO_STRING_DEFINED "N/A"

namespace SST {
namespace MemHierarchy {

class MemBackend : public SubComponent {
public:

    typedef MemBackendConvertor::ReqId ReqId;
    MemBackend();

    MemBackend(Component *comp, Params &params) : SubComponent(comp)
    {
    	output = new SST::Output("@t:MemoryBackend[@p:@l]: ", 
                params.find<uint32_t>("debug_level", 0),
                params.find<uint32_t>("debug_mask", 0),
                (Output::output_location_t)params.find<int>("debug_location", 0) );

        m_clockFreq = params.find<std::string>("clock");
	

        if ( m_clockFreq.empty() ) {
            output->fatal(CALL_INFO, -1, "MemBackend: clock is not set\n");
        }

        m_maxReqPerCycle = params.find<>("max_requests_per_cycle",-1);
        if (m_maxReqPerCycle == 0) m_maxReqPerCycle = -1;
        m_reqWidth = params.find<>("request_width",64);

        bool found;
        UnitAlgebra backendRamSize = UnitAlgebra(params.find<std::string>("mem_size", "0B", found));
        if (!found) {
            output->fatal(CALL_INFO, -1, "Param not specified (%s): backend.mem_size - memory controller must have a size specified, (NEW) WITH units. E.g., 8GiB or 1024MiB. \n", "MemBackendConvertor");
        }

        if (!backendRamSize.hasUnits("B")) {
            output->fatal(CALL_INFO, -1, "Invalid param (%s): backend.mem_size - definition has CHANGED! Now requires units in 'B' (SI OK, ex: 8GiB or 1024MiB).\nSince previous definition was implicitly MiB, you may simply append 'MiB' to the existing value. You specified '%s'\n", "MemBackendConvertor", backendRamSize.toString().c_str());
        }
        m_memSize = backendRamSize.getRoundedValue();
    }

    virtual ~MemBackend() {
        delete output;
    }

    virtual void setGetRequestorHandler( std::function<const std::string&(ReqId)> func ) {
        m_getRequestor = func;
    } 

    std::string getRequestor( ReqId id ) {
        return m_getRequestor( id );
    }

    virtual void setup() {}
    virtual void finish() {}
    
    /* Called by parent's clock() function */
    virtual bool clock(Cycle_t cycle) { return true; } 
    
    /* Interface to parent */
    virtual size_t getMemSize() { return m_memSize; }
    virtual uint32_t getRequestWidth() { return m_reqWidth; }
    virtual int32_t getMaxReqPerCycle() { return m_maxReqPerCycle; } 
    virtual const std::string& getClockFreq() { return m_clockFreq; }
    virtual bool isClocked() { return true; }
    virtual bool issueCustomRequest(ReqId, CustomCmdInfo*) {
        output->fatal(CALL_INFO, -1, "Error (%s): This backend cannot handle custom requests\n");
        return false;
    }

protected:
    Output*         output;
    std::string     m_clockFreq;
    int32_t         m_maxReqPerCycle;
    size_t          m_memSize;
    int32_t         m_reqWidth;

    std::function<const std::string&(ReqId)> m_getRequestor;
};

/* MemBackend - timing only */
class SimpleMemBackend : public MemBackend {
  public:
    SimpleMemBackend() : MemBackend() {} 
    SimpleMemBackend(Component *comp, Params &params) : MemBackend(comp,params) {}  

    virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes ) = 0; 

    void handleMemResponse( ReqId id ) {
        m_respFunc( id );
    }

    virtual void setResponseHandler( std::function<void(ReqId)> func ) {
        m_respFunc = func;
    }

  private:
    std::function<void(ReqId)> m_respFunc;
};

/* MemBackend - timing and passes request/response flags */
class FlagMemBackend : public MemBackend {
  public:
    FlagMemBackend(Component *comp, Params &params) : MemBackend(comp,params) {}  
    virtual bool issueRequest( ReqId, Addr, bool isWrite, uint32_t flags, unsigned numBytes ) = 0;

    void handleMemResponse( ReqId id, uint32_t flags ) {
        m_respFunc( id, flags );
    }

    virtual void setResponseHandler( std::function<void(ReqId,uint32_t)> func ) {
        m_respFunc = func;
    }

  private:
    std::function<void(ReqId,uint32_t)> m_respFunc;
};

class ExtMemBackend : public MemBackend {
  public:
    ExtMemBackend(Component *comp, Params &params) : MemBackend(comp,params) {}  
    virtual bool issueRequest( ReqId, Addr, bool isWrite,
                               std::vector<uint64_t> ins,
                               uint32_t flags, unsigned numBytes ) = 0;
    virtual bool issueCustomRequest( ReqId, Addr, uint32_t Cmd,
                                     std::vector<uint64_t> ins,
                                     uint32_t flags, unsigned numBytes ) = 0;

    void handleMemResponse( ReqId id, uint32_t flags ) {
        m_respFunc( id, flags );
    }

    virtual void setResponseHandler( std::function<void(ReqId,uint32_t)> func ) {
        m_respFunc = func;
    }

  private:
    std::function<void(ReqId,uint32_t)> m_respFunc;
};

}}

#endif

