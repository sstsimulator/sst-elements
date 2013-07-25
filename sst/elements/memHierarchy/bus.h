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

#ifndef _BUS_H
#define _BUS_H

#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>


#include <sst/core/interfaces/memEvent.h>

using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

class BusEvent;

class Bus : public SST::Component {
public:

    typedef MemEvent::id_type key_t;
    static const key_t ANY_KEY;
    static const char BUS_INFO_STR[];

	Bus(SST::ComponentId_t id, SST::Component::Params_t& params);
	void init(unsigned int phase);
    void printStatus(Output &out);

private:
	Bus();  // for serialization only
	Bus(const Bus&); // do not implement
	void operator=(const Bus&); // do not implement

	void requestPort(LinkId_t link_id, key_t key);
	void cancelPortRequest(LinkId_t link_id, key_t key);
	void sendMessage(BusEvent *ev, LinkId_t from_link);
    std::pair<LinkId_t, key_t>  arbitrateNext(void);

	void handleEvent( SST::Event *ev );


	void schedule(void);
	void busFinish(void);

	void handleSelfEvent( SST::Event *ev );


	class SelfEvent : public Event {
	public:
		typedef enum { Schedule, BusFinish } EventType;
		SelfEvent(EventType t) : SST::Event(), type(t) {}
		EventType type;
	};


    Output dbg;
	int numPorts;
    bool atomicDelivery;
    std::pair<LinkId_t, key_t> activePort;
	bool busBusy;
	TimeConverter *delayTC;
	SimTime_t busDelay;
	SST::Link** ports;
	SST::Link *selfLink;
	std::deque<std::pair<LinkId_t, key_t> > busRequests;
	std::map<LinkId_t, SST::Link*> linkMap;


	friend class boost::serialization::access;
	template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
			ar & BOOST_SERIALIZATION_NVP(numPorts);
			ar & BOOST_SERIALIZATION_NVP(activePort);
			ar & BOOST_SERIALIZATION_NVP(busBusy);
			ar & BOOST_SERIALIZATION_NVP(busDelay);
			for ( int i = 0 ; i < numPorts ; i++ ) {
				ar & BOOST_SERIALIZATION_NVP(ports[i]);
			}
			ar & BOOST_SERIALIZATION_NVP(selfLink);
			ar & BOOST_SERIALIZATION_NVP(busRequests);
		}

	template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
			ar & BOOST_SERIALIZATION_NVP(numPorts);
			ar & BOOST_SERIALIZATION_NVP(activePort);
			ar & BOOST_SERIALIZATION_NVP(busBusy);
			ar & BOOST_SERIALIZATION_NVP(busDelay);
			ports = new SST::Link*[numPorts];
			for ( int i = 0 ; i < numPorts ; i++ ) {
				ar & BOOST_SERIALIZATION_NVP(ports[i]);
				//resture links
				ports[i]->setFunctor(new SST::Event::Handler<Bus>(this,&Bus::handleEvent));
			}
			ar & BOOST_SERIALIZATION_NVP(selfLink);
			selfLink->setFunctor(new SST::Event::Handler<Bus>(this, &Bus::handleSelfEvent));
			ar & BOOST_SERIALIZATION_NVP(busRequests);
		}

	BOOST_SERIALIZATION_SPLIT_MEMBER()

};


class BusEvent : public SST::Event {
public:
    typedef enum { RequestBus, CancelRequest, SendData, ClearToSend } BusCommand;

    BusEvent(BusCommand cmd, Bus::key_t key) :
        cmd(cmd), key(key), payload(NULL)
    { }

    BusEvent(MemEvent *payload) :
        cmd(SendData), key(payload->getID()), payload(payload)
    { }

    BusEvent(BusEvent *be) :
        cmd(be->cmd), key(be->key), payload(be->payload)
    { }

    Bus::key_t getKey() const { return key; }
    BusCommand getCmd() const { return cmd; }
    MemEvent* getPayload() const { return payload; }


private:
    BusCommand cmd;
    Bus::key_t key;
    MemEvent *payload;


    BusEvent() {} // Serialization only

    friend class Bus;

	friend class boost::serialization::access;
	template<class Archive>
	void
	serialize(Archive & ar, const unsigned int version )
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(cmd);
        ar & BOOST_SERIALIZATION_NVP(key);
        ar & BOOST_SERIALIZATION_NVP(payload);
    }
};


};
};
#endif /* _BUS_H */
