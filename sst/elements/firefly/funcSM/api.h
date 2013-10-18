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

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/module.h>
#include <sst/core/output.h>

#include "sst/elements/hermes/msgapi.h"
#include "info.h"

namespace SST {
namespace Firefly {

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

    FunctionSMInterface( int verboseLevel, 
            Output::output_location_t loc, Info* info ) :
        m_info(info),
        m_setPrefix( true )
    {
        m_dbg.init("@t:XXXFuncSM::@p():@l ", verboseLevel, 0, loc );
    }
    virtual ~FunctionSMInterface() {} 

    virtual void  handleStartEvent( SST::Event*, Retval& ) = 0; 
    virtual void  handleEnterEvent( SST::Event*, Retval& ) { assert(0); }
    virtual void  handleSelfEvent( SST::Event*, Retval& ) { assert(0); }
    virtual const char* name() { return "No Name"; }
    virtual int myRank() { return m_info->worldRank(); }
    virtual int myNode() { return m_info->nodeId(); }

  protected:
    Info*   m_info;
    Output  m_dbg;
    bool    m_setPrefix;
};

}
}

#endif
