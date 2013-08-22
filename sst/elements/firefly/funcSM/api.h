// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_API_H 
#define COMPONENTS_FIREFLY_FUNCSM_API_H 

#include <sst_config.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/module.h>
#include <sst/core/output.h>

#include "sst/elements/hermes/msgapi.h"
#include "info.h"

namespace SST {
namespace Firefly {

class DriverEvent : public SST::Event {
  public:
    DriverEvent( Hermes::Functor* _retFunc, int _retval ) :
        Event(),
        retFunc( _retFunc ),
        retval( _retval )
    { }
    Hermes::Functor* retFunc;
    int retval;
  private:
};

class SMEnterEvent : public SST::Event {
  public:
    SMEnterEvent(int _type, Hermes::Functor* _retFunc ) :
        Event(),
        type( _type ),
        retFunc( _retFunc )
    {}

    int              type;
    SST::Link*       retLink;
    Hermes::Functor* retFunc;
};

class FunctionSMInterface : public Module {
  public:
    FunctionSMInterface( int verboseLevel, Output::output_location_t loc,
                Info* info ) :
        m_info(info),
        m_setPrefix( true )
    {
        m_dbg.init("@t:XXXFuncSM::@p():@l ", verboseLevel, 0, loc );
    }
    virtual ~FunctionSMInterface() {} 

    virtual void  handleEnterEvent( SST::Event* ) = 0; 
    virtual void  handleProgressEvent( SST::Event* ) {}
    virtual void  handleSelfEvent( SST::Event* ) {}
    virtual const char* name() { return "No Name"; }

    virtual void exit( SMEnterEvent* event, int retval ) {
        DriverEvent* x = new DriverEvent( event->retFunc, retval );
        event->retLink->send( 0,  x);
    }
    
    virtual int myRank() { return m_info->worldRank(); }
    virtual int myNode() { return m_info->nodeId(); }

  protected:
    Info*   m_info;
    Output  m_dbg;
    std::string m_xxx; 
    bool    m_setPrefix;
};

}
}

#endif
