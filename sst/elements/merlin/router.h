// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_ROUTER_H
#define COMPONENTS_MERLIN_ROUTER_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/module.h>
#include <sst/core/timeConverter.h>

using namespace SST;

namespace SST {
namespace Merlin {

const int INIT_BROADCAST_ADDR = -1;

#define MERLIN_ENABLE_TRACE
class RtrEvent : public Event {
    
public:
    int dest;
    int src;
    int vc;
    int size_in_flits;

    enum TraceType {NONE, ROUTE, FULL};

    RtrEvent() :
	Event(),
	trace(NONE)
    {}

    inline void setTraceID(int id) {traceID = id;}
    inline void setTraceType(TraceType type) {trace = type;}

    inline TraceType getTraceType() {return trace;}
    inline int getTraceID() {return traceID;}
    
private:
    TraceType trace;
    int traceID;
    
    
};


class credit_event : public Event {
public:
    int vc;
    int credits;

    credit_event(int vc, int credits) :
	Event(),
	vc(vc),
	credits(credits)
    {}
};

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

    inline void setVC(int vc) {encap_ev->vc = vc; return;}
    inline int getVC() {return encap_ev->vc;}

    inline int getFlitCount() {return encap_ev->size_in_flits;}

    inline void setEncapsulatedEvent(RtrEvent* ev) {encap_ev = ev;}
    inline RtrEvent* getEncapsulatedEvent() {return encap_ev;}

    inline int getDest() {return encap_ev->dest;}
    inline int getSrc() {return encap_ev->src;}

    inline RtrEvent::TraceType getTraceType() {return encap_ev->getTraceType();}
    inline int getTraceID() {return encap_ev->getTraceID();}
};

    class Topology : public Module {
public:
    enum PortState {R2R, R2N, UNCONNECTED};
    Topology() {}
    virtual ~Topology() {}

    virtual void route(int port, int vc, internal_router_event* ev) = 0;
    virtual internal_router_event* process_input(RtrEvent* ev) = 0;
    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts) = 0;
    virtual internal_router_event* process_InitData_input(RtrEvent* ev) = 0;
    virtual PortState getPortState(int port) const = 0;
    inline bool isHostPort(int port) const { return getPortState(port) == R2N; }
};

class PortControl;

    class XbarArbitration : public Module {
public:
    XbarArbitration() {}
    virtual ~XbarArbitration() {}

    virtual void arbitrate(PortControl** ports, int* port_busy, int* out_port_busy, int* progress_vc) = 0;
    virtual void dumpState(std::ostream& stream) {};

};

}
}

#endif // COMPONENTS_MERLIN_ROUTER_H
