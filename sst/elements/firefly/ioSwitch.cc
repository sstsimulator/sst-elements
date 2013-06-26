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
#include "sst/core/serialization/element.h"
#include "sst/core/component.h"
#include "sst/core/element.h"

#include "ioSwitch.h"
#include "ioEvent.h"

#include <cxxabi.h>

#define DBGX( fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%s::%s():%d: "fmt, realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}

using namespace SST;
using namespace SST::Firefly;

IOSwitch::IOSwitch(ComponentId_t id, Params_t &params) :
    Component( id )
{
    DBGX("\n");
    int numPorts = params.find_integer("numPorts");

    if ( -1 == numPorts ) {
        _abort(IOSwitch, "invalid number of ports %d\n", numPorts);
    }

    DBGX("numPorts %d\n", numPorts);
    m_links.resize( numPorts );

    registerTimeBase( "10 ps", true);

    for ( int i = 0; i< m_links.size(); i++ ) {
        std::ostringstream linkName;
        linkName << "port" << i;
        std::string ln = linkName.str();

        DBGX("%d %s\n",i,ln.c_str());
        m_links[i] = configureLink( ln,
            new SST::Event::Handler<IOSwitch,int>(this, &IOSwitch::handleEvent,i));
        if ( NULL == m_links[i] ) {
            _abort(IOSwitch, "configureLink failed\n");
        }
    }
}

IOSwitch::~IOSwitch()
{
    DBGX("\n");
}

void IOSwitch::init( unsigned int phase )
{
#if 0
    if ( ! phase ) {
        for ( int i = 0; i < m_links.size(); i++ ) {
            std::ostringstream portName;
            portName << i;
            std::string port = portName.str();

            SST::Event* event = new SST::Interfaces::StringEvent( port ); 
            m_links[i]->sendInitData( event ); 
        }     
    }
#endif
}

void IOSwitch::handleEvent( SST::Event* e, int src )
{
    IOEvent* event = static_cast<IOEvent*>(e);
    IO::NodeId dest = event->getNodeId();
    event->setNodeId( src );
    DBGX("src %d dest %d\n", src, dest );
    m_links[dest]->send(0,e);
}
