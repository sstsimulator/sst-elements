// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _m5bounce_h
#define _m5bounce_h

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/log.h>

#include <sst/core/component.h>

#define BOUNCE_DBG 1

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

namespace SST {
namespace M5 {

class Bounce : public SST::Component
{
    public:
        Bounce( SST::ComponentId_t id, Params_t& params );

    private:
        void eventHandler( SST::Event* e, int port );
         
        std::vector<SST::Link*>  m_linkM;
        SST::TimeConverter      *m_tc;
        SST::Log<BOUNCE_DBG>     m_dbg;
};

inline Bounce::Bounce( SST::ComponentId_t id, Params_t& params ) :
    Component( id ),
    m_dbg( "Bounce::", false )
{
    if ( params.find( "debug" ) != params.end() ) {
        if ( params["debug"].compare("yes") == 0 ) {
            m_dbg.enable();
        }
    }

    DBG("Bounce::Bounce\n");

    m_linkM.resize(2);

    m_linkM[0] = configureLink( "port0",
           new SST::Event::Handler<Bounce,int>(this, &Bounce::eventHandler, 0));

    m_linkM[1] = configureLink( "port1",
           new SST::Event::Handler<Bounce,int>(this, &Bounce::eventHandler, 1));

    m_tc = registerTimeBase("1ns");

    m_linkM[0]->setDefaultTimeBase(m_tc);
    m_linkM[1]->setDefaultTimeBase(m_tc);
}

inline void Bounce::eventHandler( SST::Event* e, int port )
{
//    m_dbg.write("%7lu: Bounce::eventHandler() port=%d\n", (unsigned long)
//                m_tc->convertToCoreTime(getCurrentSimTime(m_tc) ), port );
    m_linkM[port == 0 ? 1 : 0]->Send( e );
}

}
}

#endif

