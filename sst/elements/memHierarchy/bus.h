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

#ifndef SST_MEMHIERARCHY_BUS_H
#define SST_MEMHIERARCHY_BUS_H

#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>

#include <queue>
#include <map>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include <sst/core/interfaces/memEvent.h>

using namespace std;
using namespace SST::Interfaces;

namespace SST { namespace MemHierarchy {

class BusEvent;

class Bus : public SST::Component {
public:

    typedef MemEvent::id_type key_t;
    static const key_t ANY_KEY;
    static const char BUS_INFO_STR[];
    
	Bus(SST::ComponentId_t id, SST::Params& params);
	void init(unsigned int phase);

private:
	Bus();  // for serialization only
	Bus(const Bus&); // do not implement
	void operator=(const Bus&); // do not implement

	void processIncomingEvent(SST::Event *ev);
    void sendSingleEvent(SST::Event *ev);
    void broadcastEvent(SST::Event *ev);
    bool clockTick(Cycle_t);
    void configureParameters(SST::Params&);
    void configureLinks();
    
    void mapNodeEntry(const std::string&, LinkId_t);
    LinkId_t lookupNode(const std::string&);


    Output dbg_;
	int numHighNetPorts_;
    int numLowNetPorts_;
    int maxNumPorts_;
    int broadcast_;
    int latency_;
    
    std::string busFrequency_;
    std::string bus_latency_cycles_;
	std::vector<SST::Link*> highNetPorts_;
	std::vector<SST::Link*> lowNetPorts_;
	std::map<string, LinkId_t> nameMap_;
    std::map<LinkId_t, SST::Link*> linkIdMap_;
    std::queue<SST::Event*>   eventQueue_;
    
};

/*
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
*/


}}
#endif /* SST_MEMHIERARHCY__BUS_H */
