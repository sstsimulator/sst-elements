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


#ifndef __SST_MEMH_BACKEND__
#define __SST_MEMH_BACKEND__

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/warnmacros.h>

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

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MemHierarchy::MemBackend)

#define MEMBACKEND_ELI_PARAMS {"debug_level",     "(uint) Debugging level: 0 (no output) to 10 (all output). Output also requires that SST Core be compiled with '--enable-debug'", "0"},\
            {"debug_mask",      "(uint) Mask on debug_level", "0"},\
            {"debug_location",  "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE", "0"},\
            {"max_requests_per_cycle", "(int) Maximum number of requests to accept each cycle. Use 0 or -1 for unlimited.", "-1"},\
            {"request_width", "(int) Maximum size, in bytes, for a request", "64"},\
            {"mem_size", "(string) Size of memory with units (SI ok). E.g., '2GiB'.", NULL}

    typedef MemBackendConvertor::ReqId ReqId;
    MemBackend();

    MemBackend(ComponentId_t id, Params &params) : SubComponent(id) { 
    	output = new SST::Output("@t:MemoryBackend[@p:@l]: ",
                params.find<uint32_t>("debug_level", 0),
                params.find<uint32_t>("debug_mask", 0),
                (Output::output_location_t)params.find<int>("debug_location", 0) );

        m_maxReqPerCycle = params.find<>("max_requests_per_cycle",-1);
        if (m_maxReqPerCycle == 0) m_maxReqPerCycle = -1;
        m_reqWidth = params.find<uint32_t>("request_width",64);

        bool found;
        UnitAlgebra backendRamSize = UnitAlgebra(params.find<std::string>("mem_size", "0B", found));

        if (!backendRamSize.hasUnits("B")) {
            output->fatal(CALL_INFO, -1, "Invalid param (%s): backend.mem_size - definition has CHANGED! Now requires units in 'B' (SI OK, ex: 8GiB or 1024MiB).\nSince previous definition was implicitly MiB, you may simply append 'MiB' to the existing value. You specified '%s'\n", "MemBackendConvertor", backendRamSize.toString().c_str());
        }
        m_memSize = backendRamSize.getRoundedValue();
    }

    virtual ~MemBackend() {
        delete output;
    }

    virtual void setGetRequestorHandler( std::function<const std::string(ReqId)> func ) {
        m_getRequestor = func;
    }

    const std::string getRequestor( ReqId id ) {
        return m_getRequestor( id );
    }

    virtual void setup() {}
    virtual void finish() {}

    /* Called by parent's clock() function */
    virtual bool clock(Cycle_t UNUSED(cycle)) { return true; }

    /* Interface to parent */
    virtual size_t getMemSize() { return m_memSize; }
    virtual uint32_t getRequestWidth() { return m_reqWidth; }
    virtual int32_t getMaxReqPerCycle() { return m_maxReqPerCycle; }
    virtual bool isClocked() { return true; }
    virtual bool issueCustomRequest(ReqId, CustomCmdInfo*) {
        output->fatal(CALL_INFO, -1, "Error (%s): This backend cannot handle custom requests\n", getName().c_str());
        return false;
    }

    virtual std::string getBackendConvertorType() = 0; /* Backend must return the compatible convertor type */

protected:
    Output*         output;
    int32_t         m_maxReqPerCycle;
    size_t          m_memSize;
    uint32_t        m_reqWidth;

    std::function<const std::string(ReqId)> m_getRequestor;
};

/* MemBackend - timing only */
class SimpleMemBackend : public MemBackend {
  public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::MemHierarchy::SimpleMemBackend, SST::MemHierarchy::MemBackend)

    SimpleMemBackend() : MemBackend() {}
    SimpleMemBackend(ComponentId_t id, Params &params) : MemBackend(id,params) {}

    virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes ) = 0;

    void handleMemResponse( ReqId id ) {
        m_respFunc( id );
    }

    virtual void setResponseHandler( std::function<void(ReqId)> func ) {
        m_respFunc = func;
    }

    virtual std::string getBackendConvertorType() {
        return "memHierarchy.simpleMemBackendConvertor";
    }

  private:
    std::function<void(ReqId)> m_respFunc;
};

/* MemBackend - timing and passes request/response flags */
class FlagMemBackend : public MemBackend {
  public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::MemHierarchy::FlagMemBackend, SST::MemHierarchy::MemBackend)
    FlagMemBackend(ComponentId_t id, Params &params) : MemBackend(id,params) {}
    virtual bool issueRequest( ReqId, Addr, bool isWrite, uint32_t flags, unsigned numBytes ) = 0;

    void handleMemResponse( ReqId id, uint32_t flags ) {
        m_respFunc( id, flags );
    }

    virtual void setResponseHandler( std::function<void(ReqId,uint32_t)> func ) {
        m_respFunc = func;
    }

    virtual std::string getBackendConvertorType() {
        return "memHierarchy.flagMemBackendConvertor";
    }

  private:
    std::function<void(ReqId,uint32_t)> m_respFunc;
};

class ExtMemBackend : public MemBackend {
  public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::MemHierarchy::ExtMemBackend, SST::MemHierarchy::MemBackend)
    ExtMemBackend(ComponentId_t id, Params &params) : MemBackend(id,params) {}
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

    virtual std::string getBackendConvertorType() {
        return "memHierarchy.extMemBackendConvertor";
    }

  private:
    std::function<void(ReqId,uint32_t)> m_respFunc;
};

}}

#endif

