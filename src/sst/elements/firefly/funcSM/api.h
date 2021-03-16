// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
        void setDelay( uint64_t value ) {
            m_type = Delay;
            m_value = value;
        }
        bool isExit() { return ( Exit == m_type); }
        bool isDelay() { return ( Delay == m_type); }
        uint64_t value() { return m_value; }
      private:
        enum { None, Delay, Exit } m_type;
        uint64_t m_value;
    };

    FunctionSMInterface( SST::Params& params ) :
        m_info( NULL ),
        m_proto( NULL ),
        m_name( params.find<std::string>("name","???") ),
        m_enterLatency( params.find<int>("enterLatency",0) ),
        m_returnLatency( params.find<int>("returnLatency",0) )
    {
        char buffer[100];

        snprintf(buffer,100,"@t:%" PRId64 ":%sFunc::@p():@l ",
                    params.find<int64_t>("nodeId"),
                    m_name.c_str() );

        m_dbg.init( buffer,
            params.find<uint32_t>("verboseLevel",0),
            params.find<uint32_t>("verboseMask",-1),
            Output::STDOUT );

        m_dbg.debug(CALL_INFO,1,0,"\n");
    }

    virtual ~FunctionSMInterface() {}
    virtual void printStatus( Output& ) {}

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
};

}
}

#endif
