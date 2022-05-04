// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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

    SST_ELI_REGISTER_COMPONENT(
        Bridge,
        "merlin",
        "Bridge",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Bridge between two memory networks.",
        COMPONENT_CATEGORY_NETWORK)

    SST_ELI_DOCUMENT_PARAMS(
        {"translator",                "Translator backend.  Inherit from SST::Merlin::Bridge::Translator.", NULL},
        {"debug",                     "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
        {"debug_level",               "Debugging level: 0 to 10", "0"},
        {"network_bw",                "The network link bandwidth.", "80GiB/s"},
        {"network_input_buffer_size", "Size of the network's input buffer.", "1KiB"},
        {"network_output_buffer_size","Size of the network;s output buffer.", "1KiB"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"pkts_received_net0",           "Total number of packets recived on NIC0", "count", 1},
        {"pkts_received_net1",           "Total number of packets recived on NIC1", "count", 1},
        {"pkts_sent_net0",           "Total number of packets sent on NIC0", "count", 1},
        {"pkts_sent_net1",           "Total number of packets sent on NIC1", "count", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"network0",     "Network Link",  {} },
        {"network1",     "Network Link",  {} },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"translator", "Element to translate between the two networks.", "SST::Merlin::Bridge::Translator" },
        {"networkIF", "Element to interface to networks.  There are two IFs that need to be loaded (index 0 and 1).",
         "SST::Interfaces::SimpleNetwork" }
    )

    Bridge(SST::ComponentId_t id, SST::Params &params) :
        SST::Component(id)
    {
        int debugLevel = params.find<int>("debug_level", 0);
        dbg.init("@t:Bridge::@p():@l " + getName() + ": ",
                debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));

        Params transParams = params.get_scoped_params("translator");
        // translator = dynamic_cast<Translator*>(loadSubComponent(
        //             params.find<std::string>("translator"),
        //             this, transParams));
        translator = loadAnonymousSubComponent<Translator>(params.find<std::string>("translator"),"translator",0,
                                                           ComponentInfo::SHARE_NONE, transParams, this);
        if ( !translator ) dbg.fatal(CALL_INFO, 1, "Must specify a 'translator' subcomponent.");

        configureNIC(0, params);
        configureNIC(1, params);
    }

    ~Bridge()
    {
        for ( int i = 0 ; i < 2 ; i++ ) {
            delete interfaces[i].nic;
        }
        delete translator;
    }

    void init(unsigned int phase)
    {

        bool ready = true;
        for ( int i = 0 ; i < 2 ; i++ ) {
            Nic_t &nic = interfaces[i];
            nic.nic->init(phase);
            ready &= nic.nic->isNetworkInitialized();
        }

        translator->init(phase);

        dbg.debug(CALL_INFO, 10, 0, "Init Phase %u.  Network %sready\n", phase, ready ? "" : "NOT ");

        if ( ! ready ) return;

        for ( int i = 0 ; i < 2 ; i++ ) {
            Nic_t &nic = interfaces[i];
            Nic_t &otherNic = interfaces[i^1];


            while ( SimpleNetwork::Request *req = nic.nic->recvInitData() ) {
                dbg.debug(CALL_INFO, 2, 0, "Received init phase event on interface %d\n", i);
                SimpleNetwork::Request *res = translator->initTranslate(req, i);
                if ( res ) {
                    otherNic.nic->sendInitData(res);
                }
            }
        }
    }

    void setup(void)
    {   
        interfaces[0].nic->setup();
        interfaces[1].nic->setup();
    
        translator->setup();
    }

    void finish(void)
    {   
        interfaces[0].nic->finish();
        interfaces[1].nic->finish();
    
        translator->finish();
    }

    SimpleNetwork::nid_t getAddrForNetwork(uint8_t netID) { return interfaces[netID].getAddr(); }

    class Translator : public SST::SubComponent {
        Bridge* bridge;
    public:

        SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Merlin::Bridge::Translator, Bridge*)

        Translator(SST::ComponentId_t cid, Params &params, Bridge* bridge) : SubComponent(cid), bridge(bridge) { }
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
            return bridge->getAddrForNetwork(netID);
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

    void configureNIC(uint8_t id, SST::Params &params)
    {
        dbg.debug(CALL_INFO, 2, 0, "Initializing network interface %d\n", id);
        Nic_t &nic = interfaces[id];

        Params if_params;

        if_params.insert("link_bw",params.find<std::string>("network_bw","80GiB/s"));
        if_params.insert("input_buf_size",params.find<std::string>("network_input_buffer_size", "1KiB"));
        if_params.insert("output_buf_size",params.find<std::string>("network_output_buffer_size", "1KiB"));
        if_params.insert("port_name","network" + std::to_string(id));

        nic.nic = loadAnonymousSubComponent<SST::Interfaces::SimpleNetwork>
            ("merlin.linkcontrol", "networkIF", id,
             ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, if_params, 1 /* vns */);


        // nic.nic = (SimpleNetwork*)loadSubComponent("merlin.linkcontrol", this, params);
        // nic.nic->initialize( "network" + std::to_string(id),
        //         params.find<SST::UnitAlgebra>("network_bw", SST::UnitAlgebra("80GiB/s")),
        //         1, /* should be num VN */
        //         params.find<SST::UnitAlgebra>("network_input_buffer_size", SST::UnitAlgebra("1KiB")),
        //         params.find<SST::UnitAlgebra>("network_output_buffer_size", SST::UnitAlgebra("1KiB")));

        nic.nic->setNotifyOnReceive(new SimpleNetwork::Handler<Bridge, uint8_t>(this, &Bridge::handleIncoming, id));
        nic.nic->setNotifyOnSend(new SimpleNetwork::Handler<Bridge, uint8_t>(this, &Bridge::spaceAvailable, id));

        nic.stat_recv = registerStatistic<uint64_t>("pkts_received_net" + std::to_string(id));
        nic.stat_send = registerStatistic<uint64_t>("pkts_sent_net" + std::to_string(id));
    }

    bool handleIncoming(int vn, uint8_t id)
    {
        Nic_t &inNIC = interfaces[id];
        Nic_t &outNIC = interfaces[id^1];

        SimpleNetwork::Request* req = inNIC.nic->recv(vn);

        if ( NULL == req ) return false;
        inNIC.stat_recv->addData(1);

        dbg.debug(CALL_INFO, 5, 0, "Received event on interface %u\n", id);

        SimpleNetwork::Request *res = translator->translate(req, id);
        if ( res ) {
            if ( outNIC.nic->send(res, 0) ) {
                outNIC.stat_send->addData(1);
            } else {
                /* We failed to send. */
                outNIC.sendQueue.push_back(res);
            }
        }
        return true;
    }

    bool spaceAvailable(int vn, uint8_t id)
    {
        Nic_t &nic = interfaces[id];
        while ( !nic.sendQueue.empty() ) {
            if ( nic.nic->send(nic.sendQueue.front(), 0) ) {
                nic.stat_send->addData(1);
                nic.sendQueue.pop_front();
            } else {
                /* Not enough room yet.  */
                break;
            }
        }
        return false;
    }

};

}}

#endif
