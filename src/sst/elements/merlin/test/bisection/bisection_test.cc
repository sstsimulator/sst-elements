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
#include <sst_config.h>

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>

#include "sst/elements/merlin/merlin.h"
#include "sst/elements/merlin/test/bisection/bisection_test.h"

using namespace std;
namespace SST {
using namespace SST::Interfaces;

namespace Merlin {

bisection_test::bisection_test(ComponentId_t cid, Params& params) :
    Component(cid),
    packets_sent(0),
    packets_recd(0)
{
    // id = params.find_integer("id");
    // if ( id == -1 ) {
    // }
    // cout << "id: " << id << endl;

    num_peers = params.find<int>("num_peers");
//    cout << "num_peers: " << num_peers << endl;

    // num_vns = params.find_int("num_vns",1);;
    num_vns = 1;
//    cout << "num_vns: " << num_vns << endl;

    string link_bw_s = params.find<std::string>("link_bw","2GB/s");
    UnitAlgebra link_bw(link_bw_s);
//    cout << "link_bw: " << link_bw.toStringBestSI() << endl;
    
    string packet_size_s = params.find<std::string>("packet_size","512b");
    UnitAlgebra packet_size_ua(packet_size_s);
    
    if ( !packet_size_ua.hasUnits("b") && !packet_size_ua.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"packet_size must be specified in either "
                           "bits or bytes: %s\n",packet_size_ua.toStringBestSI().c_str());
    }
    if ( packet_size_ua.hasUnits("B") ) packet_size_ua *= UnitAlgebra("8b/B");
    packet_size = packet_size_ua.getRoundedValue();
    
    packets_to_send = params.find<int>("packets_to_send",32);
//    cout << "packets_to_send = " << packets_to_send << endl;
    
    string buffer_size_s = params.find<std::string>("buffer_size","128B");
    buffer_size = UnitAlgebra(buffer_size_s);

    // Create a LinkControl object
    std::string networkIF = params.find<std::string>("networkIF","merlin.linkcontrol");
    link_control = (SST::Interfaces::SimpleNetwork*)loadSubComponent(networkIF, this, params);

    link_control->initialize("rtr", link_bw, num_vns, buffer_size, buffer_size);

    // Set up a receive functor that will handle all incoming packets
    link_control->setNotifyOnReceive(new SST::Interfaces::SimpleNetwork::Handler<bisection_test>(this,&bisection_test::receive_handler));

    // Set up a send functor that will handle sending packets
    link_control->setNotifyOnSend(new SST::Interfaces::SimpleNetwork::Handler<bisection_test>(this,&bisection_test::send_handler));
    

    self_link = configureSelfLink("complete_link", "2GHz",
                                  new Event::Handler<bisection_test>(this,&bisection_test::handle_complete));
    
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

}

void bisection_test::setup()
{
    id = link_control->getEndpointID();
    // partner_id = (id + (num_peers / 2 )) % num_peers;
    partner_id = num_peers - id - 1;
    
    UnitAlgebra link_bw = link_control->getLinkBW();
    // Invert this to get a time per bit
    // std::cout << link_bw.toStringBestSI() << std::endl;
    link_bw.invert();
    // std::cout << link_bw.toStringBestSI() << std::endl;
    
    link_bw *= (UnitAlgebra("1b") *= packet_size);
    TimeConverter* tc = getTimeConverter(link_bw);
    // std::cout << link_bw.toStringBestSI() << std::endl;
    self_link->setDefaultTimeBase(tc);
    // std::cout << tc->getFactor() << std::endl;
    
    link_control->setup();

    // Fill up the output buffers
    send_handler(0);
}

bool
bisection_test::send_handler(int vn)
{
    // Send as many packets as we can to fill linkControl buffers
    while ( link_control->spaceToSend(vn,packet_size) && (packets_sent < packets_to_send) ) {
        bisection_test_event* ev = new bisection_test_event();
        SimpleNetwork::Request* req = new SimpleNetwork::Request();
        req->givePayload(ev);
        req->dest = partner_id;
        req->src = id;
        req->vn = vn;
        req->size_in_bits = packet_size;
        // req->setTraceID(id);
        // req->setTraceType(SST::Interfaces::SimpleNetwork::Request::FULL);
        link_control->send(req,vn);
        ++packets_sent;
    }
    if ( packets_sent == packets_to_send ) {
        return false; // Done sending
    }
    return true;
}

bool
bisection_test::receive_handler(int vn)
{
    // std::cout << "receive_handler: " << link_control->requestToReceive(vn) <<  std::endl;
    while ( link_control->requestToReceive(vn) ) {
        SimpleNetwork::Request* req = link_control->recv(vn);
        bisection_test_event* rec_ev = static_cast<bisection_test_event*>(req->takePayload());
        // cout << "received packet at " << getCurrentSimTimeNano() << endl;
        if ( packets_recd == 0 ) {
            start_time = getCurrentSimTimeNano();
            // latency = start_time - rec_ev->start_time;
        }
        ++packets_recd;

        // std::cout << id << ", " << packets_recd << std::endl;
        
        if ( packets_recd == packets_to_send ) {
            // Need to send this event to a self link to account
            // for serialization latency.  This will trigger an
            // event handler that will compute the BW.
            // std::cout << getCurrentSimTimeNano() << ", " << packet_size << std::endl;
            self_link->send(1,rec_ev); 
            delete req;
            return true;
        }
        else {
            delete req;
            delete rec_ev;
        }
    }
    return true;
}


void bisection_test::finish()
{
    link_control->finish();
}


void
bisection_test::init(unsigned int phase)
{
    link_control->init(phase);
}

 
void
bisection_test::handle_complete(Event* ev) {
    delete ev;

    // std::cout << getCurrentSimTimeNano() << std::endl;
    // Compute BW
    SimTime_t end_time = getCurrentSimTimeNano();
    
    double total_sent = (packet_size * packets_to_send);
    double total_time = (double)end_time - double (start_time);
    double bw = total_sent / total_time;

    cout << id << ":" << endl;
    // cout << "Latency = " << latency << " ns" << endl;
    cout << "  Start time = " << start_time << endl;
    cout << "  End time = " << end_time << endl;
    
    cout << "  Total sent = " << total_sent << endl;
    
    cout << "  BW = " << bw << " Gbits/sec" << endl;
    primaryComponentOKToEndSim();

}
}
}
