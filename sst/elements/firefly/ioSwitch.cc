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


#include "sst_config.h"
#include "sst/core/serialization.h"
#include "ioSwitch.h"

#include "sst/core/component.h"
#include "sst/core/element.h"
#include "sst/core/link.h"
#include "sst/core/params.h"

#include "ioEvent.h"

using namespace SST;
using namespace SST::Firefly;

IOSwitch::IOSwitch(ComponentId_t id, Params &params) :
    Component( id )
{
    int numPorts = params.find_integer("numPorts");

    m_dbg.init("@t:IOSwitch::@p():@l " + getName() + ": ",
            params.find_integer("verboseLevel",0),0,
            (Output::output_location_t)params.find_integer("debug", 0));

    if ( -1 == numPorts ) {
        _abort(IOSwitch, "invalid number of ports %d\n", numPorts);
    }

    m_dbg.verbose(CALL_INFO,1,0,"numPorts %d\n", numPorts);
    m_links.resize( numPorts );

    registerTimeBase( "10 ps", true);

    for ( unsigned int i = 0; i< m_links.size(); i++ ) {
        std::ostringstream linkName;
        linkName << "port" << i;

        m_dbg.verbose(CALL_INFO,1,0,"%d %s\n",i,linkName.str().c_str());
        m_links[i] = configureLink( linkName.str().c_str(),
            new SST::Event::Handler<IOSwitch,int>(this, &IOSwitch::handleEvent,i));
        if ( NULL == m_links[i] ) {
            _abort(IOSwitch, "configureLink failed\n");
        }
    }
}

IOSwitch::~IOSwitch()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
}

void IOSwitch::init( unsigned int phase )
{
    if ( 0 == phase ) {
        for ( unsigned int i = 0; i < m_links.size(); i++ ) {
            std::ostringstream portName;
            portName << i;
            std::string port = portName.str();

            m_dbg.verbose(CALL_INFO,1,0,"send event to port %d `%s`\n",i, port.c_str());

            SST::Event* event = new SST::Interfaces::StringEvent( port ); 
            m_links[i]->sendInitData( event ); 
        }     
    }
}

void IOSwitch::handleEvent( SST::Event* e, int src )
{
    IOEvent* event = static_cast<IOEvent*>(e);
    IO::NodeId dest = event->getNodeId();
    event->setNodeId( src );
    m_dbg.verbose(CALL_INFO,1,0,"src %d dest %d\n", src, dest );
    m_links[dest]->send(0,e);
}
