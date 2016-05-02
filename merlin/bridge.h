// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _MERLIN_BRIDGE_H_
#define _MERLIN_BRIDGE_H_


#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include <deque>


namespace SST {
namespace Merlin {

using SST::Interfaces::SimpleNetwork;

class Bridge : public SST::Component {
public:

    Bridge(SST::ComponentId_t id, SST::Params &params);
    ~Bridge();
    void init(unsigned int);
    void setup(void);
    void finish(void);

    SimpleNetwork::nid_t getAddrForNetwork(uint8_t netID) { return interfaces[netID].getAddr(); }

    class Translator : public SST::SubComponent {
    public:
        Translator(SST::Component *bridge, Params &params) : SubComponent(bridge) { }
        virtual void init(unsigned int) { }
        virtual void setup(void) { }
        virtual void finish(void) { }

        /**
         * Called when a network request is recieved.  Should return the corresponding
         * network request to be sent out on the opposite network.
         *
         * Return NULL if the packet should be be forwarded.
         */
        virtual SimpleNetwork::Request* translate(SimpleNetwork::Request* req, uint8_t fromNetwork) = 0;


        /**
         * Called when a network request is recieved during INIT.  Should return the corresponding
         * network request to be sent out on the opposite network.
         *
         * Return NULL if the packet should be be forwarded.
         */
        virtual SimpleNetwork::Request* initTranslate(SimpleNetwork::Request* req, uint8_t fromNetwork) = 0;

        SimpleNetwork::nid_t getAddrForNetwork(uint8_t netID) {
            return static_cast<Bridge*>(parent)->getAddrForNetwork(netID);
        }
    };

private:
    Output dbg;

    typedef std::map<std::string, SimpleNetwork::nid_t> addrMap_t;
    typedef std::map<std::string, uint64_t> imreMap_t;
    struct Nic_t {
        SimpleNetwork *nic;
        std::deque<SimpleNetwork::Request*> sendQueue;

        Statistic<uint64_t> *stat_recv;
        Statistic<uint64_t> *stat_send;

        SimpleNetwork::nid_t getAddr() const { return nic->getEndpointID(); }
    };

    Nic_t interfaces[2];

    Translator *translator;

    void configureNIC(uint8_t nic, SST::Params &params);
    bool handleIncoming(int vn, uint8_t nic);
    bool spaceAvailable(int vn, uint8_t nic);
};

}}

#endif
