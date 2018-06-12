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


#ifndef COMPONENTS_KINGSLEY_NOCEVENTS_H
#define COMPONENTS_KINGSLEY_NOCEVENTS_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/subcomponent.h>
#include <sst/core/timeConverter.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/interfaces/simpleNetwork.h>

using namespace SST;

namespace SST {
namespace Kingsley {

class BaseNocEvent : public Event {

public:
    enum NocEventType {CREDIT, PACKET, INTERNAL, INITIALIZATION};

    inline NocEventType getType() const { return type; }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & type;
    }    
    
protected:
    BaseNocEvent(NocEventType type) :
        Event(),
        type(type)
    {}

private:
    BaseNocEvent()  {} // For Serialization only
    NocEventType type;

    ImplementSerializable(SST::Kingsley::BaseNocEvent);
};
    

class NocPacket : public BaseNocEvent {

public:
    SST::Interfaces::SimpleNetwork::Request* request;
    int vn;
    
    NocPacket() :
        BaseNocEvent(BaseNocEvent::PACKET),
        injectionTime(0)
    {}

    NocPacket(SST::Interfaces::SimpleNetwork::Request* req) :
        BaseNocEvent(BaseNocEvent::PACKET),
        request(req),
        injectionTime(0)
    {}

    ~NocPacket()
    {
        delete request;
    }
    
    inline void setInjectionTime(SimTime_t time) {injectionTime = time;}

    virtual NocPacket* clone(void)  override {
        NocPacket *ret = new NocPacket(*this);
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
        BaseNocEvent::serialize_order(ser);
        ser & request;
        ser & vn;
        ser & size_in_flits;
        ser & injectionTime;
    }
    
private:
    SimTime_t injectionTime;
    int size_in_flits;

    ImplementSerializable(SST::Kingsley::NocPacket)
    
};

class credit_event : public BaseNocEvent {
public:
    int vn;
    int credits;

    credit_event() :
	BaseNocEvent(BaseNocEvent::CREDIT)
    {}

    credit_event(int vn, int credits) :
	BaseNocEvent(BaseNocEvent::CREDIT),
    vn(vn),
	credits(credits)
    {}

    virtual void print(const std::string& header, Output &out) const  override {
        out.output("%s credit_event to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        BaseNocEvent::serialize_order(ser);
        ser & vn;
        ser & credits;
    }
    
private:

    ImplementSerializable(SST::Kingsley::credit_event)
    
};

class NocInitEvent : public BaseNocEvent {
public:

    enum Commands { REPORT_ENDPOINT, SUM_ENDPOINTS, COMPUTE_X_SIZE, COMPUTE_Y_SIZE, COMPUTE_ENDPOINT_START,
                    BROADCAST_TOTAL_ENDPOINTS, BROADCAST_X_SIZE, BROADCAST_Y_SIZE, COMPUTE_X_POS,
                    COMPUTE_Y_POS, REPORT_ENDPOINT_ID, REPORT_FLIT_SIZE, REPORT_REQUESTED_VNS};

    Commands command;
    int int_value;
    UnitAlgebra ua_value;

    NocInitEvent() :
        BaseNocEvent(BaseNocEvent::INITIALIZATION)
    {}

    virtual NocInitEvent* clone(void)  override {
        NocInitEvent *ret = new NocInitEvent(*this);
        return ret;
    }

    virtual void print(const std::string& header, Output &out) const  override {
        out.output("%s NocInitEvent to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
        out.output("%s     command: %d, int_value = %d, ua_value = %s\n",
                   header.c_str(), command, int_value, ua_value.toStringBestSI().c_str());
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        BaseNocEvent::serialize_order(ser);
        ser & command;
        ser & int_value;
        ser & ua_value;
    }
    
    
private:
    ImplementSerializable(SST::Kingsley::NocInitEvent)
};

// class internal_router_event : public BaseNocEvent {
//     int next_port;
//     int next_vc;
//     int vc;
//     int credit_return_vc;
//     RtrEvent* encap_ev;

// public:
//     internal_router_event() :
//         BaseNocEvent(BaseNocEvent::INTERNAL)
//     {
//         encap_ev = NULL;
//     }
//     internal_router_event(RtrEvent* ev) :
//         BaseNocEvent(BaseNocEvent::INTERNAL)
//     {encap_ev = ev;}

//     virtual ~internal_router_event() {
//         if ( encap_ev != NULL ) delete encap_ev;
//     }

//     virtual internal_router_event* clone(void) override
//     {
//         return new internal_router_event(*this);
//     };

//     inline void setCreditReturnVC(int vc) {credit_return_vc = vc; return;}
//     inline int getCreditReturnVC() {return credit_return_vc;}

//     inline void setNextPort(int np) {next_port = np; return;}
//     inline int getNextPort() {return next_port;}

//     // inline void setNextVC(int vc) {next_vc = vc; return;}
//     // inline int getNextVC() {return next_vc;}

//     inline void setVC(int vc_in) {vc = vc_in; return;}
//     inline int getVC() {return vc;}

//     inline void setVN(int vn) {encap_ev->request->vn = vn; return;}
//     inline int getVN() {return encap_ev->request->vn;}

//     inline int getFlitCount() {return encap_ev->getSizeInFlits();}

//     inline void setEncapsulatedEvent(RtrEvent* ev) {encap_ev = ev;}
//     inline RtrEvent* getEncapsulatedEvent() {return encap_ev;}

//     inline int getDest() const {return encap_ev->request->dest;}
//     inline int getSrc() const {return encap_ev->request->src;}

//     inline SST::Interfaces::SimpleNetwork::Request::TraceType getTraceType() {return encap_ev->getTraceType();}
//     inline int getTraceID() {return encap_ev->getTraceID();}

//     virtual void print(const std::string& header, Output &out) const  override {
//         out.output("%s internal_router_event to be delivered at %" PRIu64 " with priority %d.  src = %d, dest = %d\n",
//                    header.c_str(), getDeliveryTime(), getPriority(), getSrc(), getDest());
//         if ( encap_ev != NULL ) encap_ev->print(header + std::string("-> "),out);
//     }

//     void serialize_order(SST::Core::Serialization::serializer &ser)  override {
//         BaseNocEvent::serialize_order(ser);
//         ser & next_port;
//         ser & next_vc;
//         ser & vc;
//         ser & credit_return_vc;
//         ser & encap_ev;
//     }
    
// private:
//     ImplementSerializable(SST::Kingsley::internal_router_event)
// };

}
}

#endif // COMPONENTS_KINGSLEY_NOCEVENTS_H
