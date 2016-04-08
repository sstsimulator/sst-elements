// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _m5bounce_h
#define _m5bounce_h

#include <sst/core/output.h>

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#define BOUNCE_DBG 1

namespace SST {
namespace M5 {

class Bounce : public SST::Component
{
    public:
        Bounce( SST::ComponentId_t id, Params& params );

    private:
        void eventHandler( SST::Event* e, int port );
         
        std::vector<SST::Link*>  m_linkM;
        SST::TimeConverter      *m_tc;
        SST::Output              m_dbg;
};

inline Bounce::Bounce( SST::ComponentId_t id, Params& params ) :
    Component( id )
{
    m_dbg.init("@t:Bounce::@p():@l " + getName() + ": ", 0, 0,
            (Output::output_location_t)params.find_integer("debug", 0));

    m_dbg.output(CALL_INFO, "Bounce::Bounce\n");

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
    m_linkM[port == 0 ? 1 : 0]->send( e ); 
}

}
}

#endif

