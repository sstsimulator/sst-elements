// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_API_H 
#define COMPONENTS_FIREFLY_FUNCSM_API_H 

#include <sst/core/event.h>
#include <sst/core/module.h>
#include <sst/core/output.h>
#include <sst/core/params.h>

#include "sst/elements/hermes/msgapi.h"
#include "ctrlMsgFunctors.h"

namespace SST {
namespace Firefly {

class Info;
class ProtocolAPI;

class FunctionSMInterface : public Module {

  public:
    class Retval {
      public:
        Retval() : m_type( None ) {}
        void setExit( int value ) {
            m_type = Exit;
            m_value = value;
        }
        void setDelay( int value ) {
            m_type = Delay;
            m_value = value;
        }
        bool isExit() { return ( Exit == m_type); }
        bool isDelay() { return ( Delay == m_type); }
        int value() { return m_value; }
      private:
        enum { None, Delay, Exit } m_type;  
        int m_value;
    };

    FunctionSMInterface( SST::Params& params ) :
        m_info( NULL ),
        m_proto( NULL ),
        m_name( params.find_string("name","???") ),
        m_enterLatency( params.find_integer("enterLatency",0) ),
        m_returnLatency( params.find_integer("returnLatency",0) )
    {
        char buffer[100];

        snprintf(buffer,100,"@t:%ld:%sFunc::@p():@l ",
                    params.find_integer("nodeId"), 
                    m_name.c_str() );

        m_dbg.init( buffer, params.find_integer("verbose",0), 0,
            (Output::output_location_t)params.find_integer("debug", 0) );
    
        m_dbg.verbose(CALL_INFO,1,0,"\n");
    }

    virtual ~FunctionSMInterface() {} 
    virtual void printStatus( Output& ) {}
    void setBackToMe( CtrlMsg::FunctorBase_0<bool>* functor ) {
        m_backToMe = functor;
    }

    void setInfo( Info* info ) { m_info = info; }
    void setProtocol( ProtocolAPI* proto ) { m_proto = proto; }
    virtual void  handleStartEvent( SST::Event*, Retval& ) = 0; 
    virtual void  handleEnterEvent( Retval& ) { assert(0); }
    virtual std::string  name() { return m_name; }
    virtual int enterLatency() { return m_enterLatency; }
    virtual int returnLatency() { return m_returnLatency; }
    virtual std::string protocolName() { return ""; }

  protected:
    Info*           m_info;
    ProtocolAPI*    m_proto;
    Output          m_dbg;
    std::string     m_name;
    int             m_enterLatency;
    int             m_returnLatency;
    CtrlMsg::FunctorBase_0<bool>* m_backToMe;
};

}
}

#endif
