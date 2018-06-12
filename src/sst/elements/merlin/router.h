// -*- mode: c++ -*-

// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_ROUTER_H
#define COMPONENTS_MERLIN_ROUTER_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/subcomponent.h>
#include <sst/core/timeConverter.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/interfaces/simpleNetwork.h>

using namespace SST;

namespace SST {
namespace Merlin {

#define VERIFY_DECLOCKING 0
    
const int INIT_BROADCAST_ADDR = -1;

class TopologyEvent;
    
class Router : public Component {
private:
    bool requestNotifyOnEvent;

    Router() :
    	Component(),
    	requestNotifyOnEvent(false),
    	vcs_with_data(0)
    {}

protected:
    inline void setRequestNotifyOnEvent(bool state)
    { requestNotifyOnEvent = state; }

    int vcs_with_data;
    
public:

    Router(ComponentId_t id) :
        Component(id),
        requestNotifyOnEvent(false),
        vcs_with_data(0)
    {}

    virtual ~Router() {}
    
    inline bool getRequestNotifyOnEvent() { return requestNotifyOnEvent; }
   
    virtual void notifyEvent() {}

    inline void inc_vcs_with_data() { vcs_with_data++; }
    inline void dec_vcs_with_data() { vcs_with_data--; }
    inline int get_vcs_with_data() { return vcs_with_data; }

    virtual int const* getOutputBufferCredits() = 0;
    virtual void sendTopologyEvent(int port, TopologyEvent* ev) = 0;
    virtual void recvTopologyEvent(int port, TopologyEvent* ev) = 0;

    virtual void reportRequestedVNs(int port, int vns) = 0;
    virtual void reportSetVCs(int port, int vcs) = 0;
};

#define MERLIN_ENABLE_TRACE


class BaseRtrEvent : public Event {

public:
    enum RtrEventType {CREDIT, PACKET, INTERNAL, TOPOLOGY, INITIALIZATION};

    inline RtrEventType getType() const { return type; }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & type;
    }    
    
protected:
    BaseRtrEvent(RtrEventType type) :
        Event(),
        type(type)
    {}

private:
    BaseRtrEvent()  {} // For Serialization only
    RtrEventType type;

    ImplementSerializable(SST::Merlin::BaseRtrEvent);
};
    

class RtrEvent : public BaseRtrEvent {

public:
    SST::Interfaces::SimpleNetwork::Request* request;
    
    RtrEvent() :
        BaseRtrEvent(BaseRtrEvent::PACKET),
        injectionTime(0)
    {}

    RtrEvent(SST::Interfaces::SimpleNetwork::Request* req) :
        BaseRtrEvent(BaseRtrEvent::PACKET),
        request(req),
        injectionTime(0)
    {}

    ~RtrEvent()
    {
        delete request;
    }
    
    inline void setInjectionTime(SimTime_t time) {injectionTime = time;}
    // inline void setTraceID(int id) {traceID = id;}
    // inline void setTraceType(TraceType type) {trace = type;}
    virtual RtrEvent* clone(void)  override {
        RtrEvent *ret = new RtrEvent(*this);
        ret->request = this->request->clone();
        return ret;
    }

    inline SimTime_t getInjectionTime(void) const { return injectionTime; }
    inline SST::Interfaces::SimpleNetwork::Request::TraceType getTraceType() const {return request->getTraceType();}
    inline int getTraceID() const {return request->getTraceID();}
    
    inline void setSizeInFlits(int size ) {size_in_flits = size; }
    inline int getSizeInFlits() { return size_in_flits; }

    virtual void print(const std::string& header, Output &out) const  override {
        out.output("%s RtrEvent to be delivered at %" PRIu64 " with priority %d. src = %lld, dest = %lld\n",
                   header.c_str(), getDeliveryTime(), getPriority(), request->src, request->dest);
        if ( request->inspectPayload() != NULL) request->inspectPayload()->print("  -> ", out);
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        BaseRtrEvent::serialize_order(ser);
        ser & request;
        ser & size_in_flits;
        ser & injectionTime;
    }
    
private:
    // TraceType trace;
    // int traceID;
    SimTime_t injectionTime;
    int size_in_flits;

    ImplementSerializable(SST::Merlin::RtrEvent)
    
};

class TopologyEvent : public BaseRtrEvent {
    // Allows Topology events to consume bandwidth.  If this is set to
    // zero, then no bandwidth is consumed.
    int size_in_flits;
    
public:
    TopologyEvent(int size_in_flits) :
	BaseRtrEvent(BaseRtrEvent::TOPOLOGY),
	size_in_flits(size_in_flits)
    {}

    TopologyEvent() :
	BaseRtrEvent(BaseRtrEvent::TOPOLOGY),
	size_in_flits(0)
    {}

    inline void setSizeInFlits(int size) { size_in_flits = size; }
    inline int getSizeInFlits() { return size_in_flits; }

    virtual void print(const std::string& header, Output &out) const  override {
        out.output("%s TopologyEvent to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        BaseRtrEvent::serialize_order(ser);
        ser & size_in_flits;
    }
    
    ImplementSerializable(SST::Merlin::TopologyEvent);
};

class credit_event : public BaseRtrEvent {
public:
    int vc;
    int credits;

    credit_event() :
	BaseRtrEvent(BaseRtrEvent::CREDIT)
    {}

    credit_event(int vc, int credits) :
	BaseRtrEvent(BaseRtrEvent::CREDIT),
	vc(vc),
	credits(credits)
    {}

    virtual void print(const std::string& header, Output &out) const  override {
        out.output("%s credit_event to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        BaseRtrEvent::serialize_order(ser);
        ser & vc;
        ser & credits;
    }
    
private:

    ImplementSerializable(SST::Merlin::credit_event)
    
};

class RtrInitEvent : public BaseRtrEvent {
public:

    enum Commands { REQUEST_VNS, SET_VCS, REPORT_ID, REPORT_BW, REPORT_FLIT_SIZE, REPORT_PORT };

    // int num_vns;
    // int id;

    Commands command;
    int int_value;
    UnitAlgebra ua_value;

    RtrInitEvent() :
        BaseRtrEvent(BaseRtrEvent::INITIALIZATION)
    {}

    virtual void print(const std::string& header, Output &out) const  override {
        out.output("%s RtrInitEvent to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
        out.output("%s     command: %d, int_value = %d, ua_value = %s\n",
                   header.c_str(), command, int_value, ua_value.toStringBestSI().c_str());
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        BaseRtrEvent::serialize_order(ser);
        ser & command;
        ser & int_value;
        ser & ua_value;
    }
    
    
private:
    ImplementSerializable(SST::Merlin::RtrInitEvent)
};

class internal_router_event : public BaseRtrEvent {
    int next_port;
    int next_vc;
    int vc;
    int credit_return_vc;
    RtrEvent* encap_ev;

public:
    internal_router_event() :
        BaseRtrEvent(BaseRtrEvent::INTERNAL)
    {
        encap_ev = NULL;
    }
    internal_router_event(RtrEvent* ev) :
        BaseRtrEvent(BaseRtrEvent::INTERNAL)
    {encap_ev = ev;}

    virtual ~internal_router_event() {
        if ( encap_ev != NULL ) delete encap_ev;
    }

    virtual internal_router_event* clone(void) override
    {
        return new internal_router_event(*this);
    };

    inline void setCreditReturnVC(int vc) {credit_return_vc = vc; return;}
    inline int getCreditReturnVC() {return credit_return_vc;}

    inline void setNextPort(int np) {next_port = np; return;}
    inline int getNextPort() {return next_port;}

    // inline void setNextVC(int vc) {next_vc = vc; return;}
    // inline int getNextVC() {return next_vc;}

    inline void setVC(int vc_in) {vc = vc_in; return;}
    inline int getVC() {return vc;}

    inline void setVN(int vn) {encap_ev->request->vn = vn; return;}
    inline int getVN() {return encap_ev->request->vn;}

    inline int getFlitCount() {return encap_ev->getSizeInFlits();}

    inline void setEncapsulatedEvent(RtrEvent* ev) {encap_ev = ev;}
    inline RtrEvent* getEncapsulatedEvent() {return encap_ev;}

    inline int getDest() const {return encap_ev->request->dest;}
    inline int getSrc() const {return encap_ev->request->src;}

    inline SST::Interfaces::SimpleNetwork::Request::TraceType getTraceType() {return encap_ev->getTraceType();}
    inline int getTraceID() {return encap_ev->getTraceID();}

    virtual void print(const std::string& header, Output &out) const  override {
        out.output("%s internal_router_event to be delivered at %" PRIu64 " with priority %d.  src = %d, dest = %d\n",
                   header.c_str(), getDeliveryTime(), getPriority(), getSrc(), getDest());
        if ( encap_ev != NULL ) encap_ev->print(header + std::string("-> "),out);
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        BaseRtrEvent::serialize_order(ser);
        ser & next_port;
        ser & next_vc;
        ser & vc;
        ser & credit_return_vc;
        ser & encap_ev;
    }
    
private:
    ImplementSerializable(SST::Merlin::internal_router_event)
};

class Topology : public SubComponent {
public:
    enum PortState {R2R, R2N, UNCONNECTED};
    Topology(Component* comp) : SubComponent(comp), output(Simulation::getSimulation()->getSimulationOutput()) {}
    virtual ~Topology() {}

    virtual void route(int port, int vc, internal_router_event* ev) = 0;
    virtual void reroute(int port, int vc, internal_router_event* ev)  { route(port,vc,ev); }
    virtual internal_router_event* process_input(RtrEvent* ev) = 0;
	
    // Returns whether the port is a router to router, router to nic, or unconnected
    virtual PortState getPortState(int port) const = 0;
    inline bool isHostPort(int port) const { return getPortState(port) == R2N; }
    virtual std::string getPortLogicalGroup(int port) const {return "";}
    
    // Methods used during init phase to route init messages
    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts) = 0;
    virtual internal_router_event* process_InitData_input(RtrEvent* ev) = 0;

    // Method used for autodiscovery of VC/VN
    virtual int computeNumVCs(int vns) {return vns;}
    // Method used to set endpoint ID
    virtual int getEndpointID(int port) {return -1;}
    
    // Sets the array that holds the credit values for all the output
    // buffers.  Format is:
    // For port=n, VC=x, location in array is n*num_vcs + x.

    // If topology does not need this information, then default
    // version will ignore it.  If topology needs the information, it
    // will need to overload function to store it.
    virtual void setOutputBufferCreditArray(int const* array, int vcs) {};
    virtual void setOutputQueueLengthsArray(int const* array, int vcs) {};
	
    // When TopologyEvents arrive, they are sent directly to the
    // topology object for the router
    virtual void recvTopologyEvent(int port, TopologyEvent* ev) {};
    
protected:
    Output &output;
};

class PortControl;

class XbarArbitration : public SubComponent {
public:
    XbarArbitration(Component* parent) :
        SubComponent(parent)
    {}
    virtual ~XbarArbitration() {}

#if VERIFY_DECLOCKING
    virtual void arbitrate(PortControl** ports, int* port_busy, int* out_port_busy, int* progress_vc, bool clocking) = 0;
#else
    virtual void arbitrate(PortControl** ports, int* port_busy, int* out_port_busy, int* progress_vc) = 0;
#endif
    virtual void setPorts(int num_ports, int num_vcs) = 0;
    virtual bool isOkayToPauseClock() { return true; }
    virtual void reportSkippedCycles(Cycle_t cycles) {};
    virtual void dumpState(std::ostream& stream) {};
	
};

}
}

#endif // COMPONENTS_MERLIN_ROUTER_H
