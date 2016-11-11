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


#ifndef __SST_MEMH_BACKEND__
#define __SST_MEMH_BACKEND__

#include <sst/core/event.h>
#include <sst/core/output.h>

#include <iostream>
#include <map>

#include "sst/elements/memHierarchy/membackend/memBackendConvertor.h"
#include "sst/elements/memHierarchy/membackend/simpleMemBackendConvertor.h"
#include "sst/elements/memHierarchy/membackend/hmcMemBackendConvertor.h"

#define NO_STRING_DEFINED "N/A"

namespace SST {
namespace MemHierarchy {

class MemBackend : public SubComponent {
public:
    class NotifyFunctorBase {
    public:
        virtual ~NotifyFunctorBase() {}
    };

    template < class T1, class A1, class TRetval = void >
    class NotifyFunctor_1 : public NotifyFunctorBase {
      private:
        T1* m_obj;
        TRetval ( T1::*m_fptr )( A1 );

      public:
        NotifyFunctor_1( T1* obj, TRetval (T1::*fptr)(A1) ) :
            m_obj( obj ),
            m_fptr( fptr )
        {}

        virtual TRetval operator()( A1 a1 ) {
            return (*m_obj.*m_fptr)( a1  );
        }
    };

    template < class T1, class A1, class A2, class TRetval = void >
    class NotifyFunctor_2 : public NotifyFunctorBase {
      private:
        T1* m_obj;
        TRetval ( T1::*m_fptr )( A1, A2 );

      public:
        NotifyFunctor_2( T1* obj, TRetval (T1::*fptr)(A1,A2) ) :
            m_obj( obj ),
            m_fptr( fptr )
        {}

        virtual TRetval operator()( A1 a1, A2 a2 ) {
            return (*m_obj.*m_fptr)( a1, a2 );
        }
    };

    typedef MemBackendConvertor::ReqId ReqId;
    MemBackend();

    MemBackend(Component *comp, Params &params) : SubComponent(comp), m_handleMemResponse(NULL)
    {
    	output = new SST::Output("@t:MemoryBackend[@p:@l]: ", 
                params.find<uint32_t>("debug_level", 0),
                params.find<uint32_t>("debug_mask", 0),
                (Output::output_location_t)params.find<int>("debug_location", 0) );

        m_clockFreq = params.find<std::string>("clock");

        if ( m_clockFreq.empty() ) {
            output->fatal(CALL_INFO, -1, "MemBackend: clock is not set\n");
        }

        m_maxReqPerCycle = params.find<>("maxReqPerCycle",-1);
        m_reqWidth = params.find<>("reqWidth",64);

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

    virtual void setResponseHandler( NotifyFunctorBase* functor ) {
        m_handleMemResponse = functor;
    } 

    virtual void setGetRequestorHandler( NotifyFunctor_1<MemBackendConvertor,ReqId,const std::string&>* functor ) {
        m_getRequestor = functor;
    } 

    std::string getRequestor( ReqId id ) {
        return (*m_getRequestor)( id );
    }

    virtual void setup() {}
    virtual void finish() {}
    virtual void clock() {} 
    virtual size_t getMemSize() { return m_memSize; }
    virtual uint32_t getRequestWidth() { return m_reqWidth; }
    virtual int32_t getMaxReqPerCycle() { return m_maxReqPerCycle; } 
    virtual const std::string& getClockFreq() { return m_clockFreq; } 
protected:
    Output*         output;
    std::string     m_clockFreq;
    int32_t         m_maxReqPerCycle;
    size_t          m_memSize;
    int32_t         m_reqWidth;

    NotifyFunctorBase*  m_handleMemResponse; 
    NotifyFunctor_1<MemBackendConvertor,ReqId,const std::string&>*  m_getRequestor; 
};

class SimpleMemBackend : public MemBackend {
  public:
    SimpleMemBackend() : MemBackend() {} 
    SimpleMemBackend(Component *comp, Params &params) : MemBackend(comp,params) {}  

    virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes ) = 0; 

    void handleMemResponse( ReqId id ) {
           
        NotifyFunctor_1< SimpleMemBackendConvertor,ReqId>*  tmp;
        tmp = static_cast< NotifyFunctor_1< SimpleMemBackendConvertor,ReqId>* >( m_handleMemResponse ); 
        (*tmp)( id );  
    }
};

class HMCMemBackend : public MemBackend {
  public:
    HMCMemBackend(Component *comp, Params &params) : MemBackend(comp,params) {}  
    virtual bool issueRequest( ReqId, Addr, bool isWrite, uint32_t flags, unsigned numBytes ) = 0;

    void handleMemResponse( ReqId id, uint32_t flags ) {

        NotifyFunctor_2< HMCMemBackendConvertor,ReqId,uint32_t>*  tmp;
        tmp = static_cast< NotifyFunctor_2< HMCMemBackendConvertor,ReqId,uint32_t>* >( m_handleMemResponse ); 
        (*tmp)( id, flags );  
    }
};

}}

#endif

