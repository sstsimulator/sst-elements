// -*- mode: c++ -*-

// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_ROUTER_H
#define COMPONENTS_MERLIN_ROUTER_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include "RtrIF.h"

using namespace SST;

class internal_router_event : public Event {
    int next_port;
    int next_vc;
    RtrEvent* encap_ev;

public:
    internal_router_event() {}
    internal_router_event(RtrEvent* ev) {encap_ev = ev;}
    virtual ~internal_router_event() {
	if ( encap_ev != NULL ) delete encap_ev;
    }

    inline void setNextPort(int np) {next_port = np; return;}
    inline int getNextPort() {return next_port;}

    inline void setEncapsulatedEvent(RtrEvent* ev) {encap_ev = ev;}
    inline RtrEvent* getEncapsulatedEvent() {return encap_ev;}
};

class Topology {
public:
    Topology() {}
    ~Topology() {}
    
    virtual void route(int port, int vc, internal_router_event* ev) = 0;
    virtual internal_router_event* process_input(RtrEvent* ev) = 0;
    virtual bool isHostPort(int port) = 0;
};

#endif // COMPONENTS_MERLIN_ROUTER_H
