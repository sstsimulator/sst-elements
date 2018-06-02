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
#include "noc_mesh.h"

#include <sst/core/params.h>
#include <sst/core/sharedRegion.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>

#include <algorithm>
#include <sstream>
#include <string>

#include <signal.h>

#include "nocEvents.h"

using namespace SST::Kingsley;
using namespace SST::Interfaces;
using namespace std;


// Start class functions
noc_mesh::~noc_mesh()
{
}

noc_mesh::noc_mesh(ComponentId_t cid, Params& params) :
    Component(cid),
    init_state(0),
    init_count(0),
    endpoint_start(0),
    total_endpoints(0),
    edge_status(0),
    endpoint_locations(0),
    use_dense_map(false),
    dense_map(NULL),
    output(Simulation::getSimulation()->getSimulationOutput())
{
    // Get the options for the router
    local_ports = params.find<int>("local_ports",1);

    use_dense_map = params.find<bool>("use_dense_map",false);

    port_priority_equal = params.find<bool>("port_priority_equal",false);
    
    // Parse all the timing parameters

    bool found = false;

    // Flit size
    UnitAlgebra flit_size_ua = params.find<UnitAlgebra>("flit_size",found);
    if ( !found ) {
        output.fatal(CALL_INFO, -1, "noc_mesh requires flit_size to be specified\n");
    }
    if ( flit_size_ua.hasUnits("B") ) {
        // Need to convert to bits per second
        flit_size_ua *= UnitAlgebra("8b/B");
    }
    flit_size = flit_size_ua.getRoundedValue();

    UnitAlgebra input_buf_size_ua = params.find<UnitAlgebra>("input_buf_size",flit_size_ua * 2);
    if ( input_buf_size_ua.hasUnits("B") ) {
        // Need to convert to bits per second
        input_buf_size_ua *= UnitAlgebra("8b/B");
    }
    input_buf_size = input_buf_size_ua.getRoundedValue();


    UnitAlgebra link_bw_ua = params.find<UnitAlgebra>("link_bw",found);
    if ( !found ) {
        output.fatal(CALL_INFO, -1, "noc_mesh requires link_bw to be specified\n");
    }
    if ( link_bw_ua.hasUnits("B/s") ) {
        // Need to convert to bits per second
        link_bw_ua *= UnitAlgebra("8b/B");
    }
    
    UnitAlgebra clock_freq = link_bw_ua / flit_size_ua;

    route_y_first = params.find<bool>("route_y_first",false);
    
    // Register the clock
    my_clock_handler = new Clock::Handler<noc_mesh>(this,&noc_mesh::clock_handler);
    clock_tc = registerClock( clock_freq, my_clock_handler);
    clock_is_off = false;
    
    // Configure the ports
    ports = new Link*[local_port_start + local_ports];

    // Configure directional ports
    Event::Handler<noc_mesh,int>* dummy_handler = new Event::Handler<noc_mesh,int>(this,&noc_mesh::handle_input_r2r,-1);

    last_time = 0;

    // Configure all the links and add all the statistics
    send_bit_count = new Statistic<uint64_t>*[local_ports + 4];
    output_port_stalls = new Statistic<uint64_t>*[local_ports + 4];
    xbar_stalls = new Statistic<uint64_t>*[local_ports + 4];


    // North port
    ports[north_port] = configureLink("north", dummy_handler);
    // stats
    send_bit_count[north_port] = registerStatistic<uint64_t>("send_bit_count","north");
    output_port_stalls[north_port] = registerStatistic<uint64_t>("output_port_stalls","north");
    xbar_stalls[north_port] = registerStatistic<uint64_t>("xbar_stalls","north");

    // South port
    ports[south_port] = configureLink("south", dummy_handler);
    // stats
    send_bit_count[south_port] = registerStatistic<uint64_t>("send_bit_count","south");
    output_port_stalls[south_port] = registerStatistic<uint64_t>("output_port_stalls","south");
    xbar_stalls[south_port] = registerStatistic<uint64_t>("xbar_stalls","south");

    // East port
    ports[east_port] = configureLink("east", dummy_handler);
    // stats
    send_bit_count[east_port] = registerStatistic<uint64_t>("send_bit_count","east");
    output_port_stalls[east_port] = registerStatistic<uint64_t>("output_port_stalls","east");
    xbar_stalls[east_port] = registerStatistic<uint64_t>("xbar_stalls","east");

    // West port
    ports[west_port] = configureLink("west", dummy_handler);
    // stats
    send_bit_count[west_port] = registerStatistic<uint64_t>("send_bit_count","west");
    output_port_stalls[west_port] = registerStatistic<uint64_t>("output_port_stalls","west");
    xbar_stalls[west_port] = registerStatistic<uint64_t>("xbar_stalls","west");

    // Configure local ports
    for ( int i = 0; i < local_ports; ++i ) {
        std::stringstream port_name;
        port_name << "local";
        port_name << i;
        ports[local_port_start + i] =
            configureLink(port_name.str(),
                          // new Event::Handler<noc_mesh,int>(this,&noc_mesh::handle_input_ep2r,local_port_start+i));
                          dummy_handler);

        // stats
        send_bit_count[local_port_start + i] = registerStatistic<uint64_t>("send_bit_count",port_name.str());
        output_port_stalls[local_port_start + i] = registerStatistic<uint64_t>("output_port_stalls",port_name.str());
        xbar_stalls[local_port_start + i] = registerStatistic<uint64_t>("xbar_stalls",port_name.str());
    }

    
    // Allocate space for all the input buffers
    port_queues = new port_queue_t[local_port_start + local_ports];
    port_busy = new int[local_port_start + local_ports];
    for ( int i = 0; i < local_port_start + local_ports; ++i ) {
        port_busy[i] = 0;
    }

    port_credits = new int[local_port_start + local_ports];
    for ( int i = 0; i < local_port_start + local_ports; ++i ) {
        port_credits[i] = 0;
    }
}

void
noc_mesh::route(noc_mesh_event* event)
{
    if ( route_y_first ) {
        // Compute next port
        if ( event->dest_mesh_loc.second > my_y ) {
            event->next_port = north_port;
        }
        else if ( event->dest_mesh_loc.second < my_y ) {
            event->next_port = south_port;
        }
        else {
            if ( event->dest_mesh_loc.first > my_x ) {
                event->next_port = east_port;
            }
            else if ( event->dest_mesh_loc.first < my_x) {
                event->next_port = west_port;
            }
            else {
                event->next_port = event->egress_port;
            }
        }    
    }

    else {
        // Compute next port
        if ( event->dest_mesh_loc.first > my_x ) {
            event->next_port = east_port;
        }
        else if ( event->dest_mesh_loc.first < my_x) {
            event->next_port = west_port;
        }
        else {
            if ( event->dest_mesh_loc.second > my_y ) {
                event->next_port = north_port;
            }
            else if ( event->dest_mesh_loc.second < my_y) {
                event->next_port = south_port;
            }
            else {
                event->next_port = event->egress_port;
            }
        }
    }
}


void
noc_mesh::handle_input_r2r(Event* ev, int port)
{
    // TraceFunction trace(CALL_INFO);
    // Check type of event
    BaseNocEvent* base_ev = static_cast<BaseNocEvent*>(ev);
    switch ( base_ev->getType() ) {
    case BaseNocEvent::CREDIT:
    {
        credit_event* credit_ret = static_cast<credit_event*>(ev);
        port_credits[port] += credit_ret->credits;
        // output.output("(%d,%d): Got credit event for VN %d with %d credits\n",my_x,my_y,credit_ret->vn,credit_ret->credits);
        delete ev;
        break;
    }
    case BaseNocEvent::INTERNAL:
    {
        noc_mesh_event* event = static_cast<noc_mesh_event*>(ev);

        route(event);
        
        // Put the event into the proper queue
        port_queues[port].push(event);
        if (clock_is_off) 
            clock_wakeup();
        break;
    }
    default:
        break;        
    }
    
}

noc_mesh_event*
noc_mesh::wrap_incoming_packet(NocPacket* packet) {
    // Wrap the incoming NocPacket in a noc_mesh_event
    noc_mesh_event* event = new noc_mesh_event(packet);
    
    // Compute the destination router
    int dest = packet->request->dest;

    if ( dest == SimpleNetwork::INIT_BROADCAST_ADDR ) {
        event->dest_mesh_loc.first = -1;
        event->dest_mesh_loc.second = -1;
        event->egress_port = -1;
        return event;
    }

    // Check to see if we have dense addressing
    if ( use_dense_map ) {
        dest = dense_map[dest];
        
    }
    
    int dest_rtr_id = dest / local_ports;
    int x = dest_rtr_id % x_size;
    int y = dest_rtr_id / x_size;

    // Compute the egress port.  If this is in the halo, then it will
    // be either north, south, east or west.  If it is not in the halo,
    // it will be one of the local_ports.
    if ( x == 0 ) {
        x = 1;
        event->egress_port = west_port;
    }
    else if ( x == x_size - 1) {
        x = x_size - 2;
        event->egress_port = east_port;
    }
    else if ( y == 0 ) {
        y = 1;
        event->egress_port = south_port;
    }
    else if ( y == y_size - 1 ) {
        y = y_size - 2;
        event->egress_port = north_port;
    }
    else {
        event->egress_port = local_port_start + (dest - (((y * x_size) + x ) * local_ports) );
        // output.output("****** Setting egress port to: %d -> %d, %d, %d, %d\n",event->egress_port,dest,x,x_size,y);
    }
    
    event->dest_mesh_loc.first = x;
    event->dest_mesh_loc.second = y;

    // if ( packet->request->dest == 15 || packet->request->dest == 24 ) {
    //     output.output("dest %lld (%d) routed to router (%d,%d) with egress %d\n",packet->request->dest,dest,x,y,event->egress_port);
    // }
    
    return event;
}

void
noc_mesh::handle_input_ep2r(Event* ev, int port)
{
    // TraceFunction trace(CALL_INFO);
    // Check type of event
    BaseNocEvent* base_ev = static_cast<BaseNocEvent*>(ev);
    switch ( base_ev->getType() ) {
    case BaseNocEvent::CREDIT:
    {
        credit_event* credit_ret = static_cast<credit_event*>(ev);
        port_credits[port] += credit_ret->credits;
        delete ev;
        break;
    }
    case BaseNocEvent::PACKET:
    {
        NocPacket* packet = static_cast<NocPacket*>(ev);
        
        // Wrap the incoming NocPacket in a noc_mesh_event
        noc_mesh_event* event = wrap_incoming_packet(packet);
        route(event); 
   
        // Need to put the event into the proper queue
        port_queues[port].push(event);
        if (clock_is_off)
            clock_wakeup();
        break;
    }
    default:
        break;        
    }
}


// void
// noc_mesh::printStatus(Output& out)
// {
//     out.output("Start Router:  id = %d\n", id);
//     for ( int i = 0; i < num_ports; i++ ) {
//         ports[i]->printStatus(out, out_port_busy[i], in_port_busy[i]);
//     }
//     out.output("End Router: id = %d\n", id);
// }

void noc_mesh::clock_wakeup() {
    Cycle_t time = reregisterClock(clock_tc, my_clock_handler);
    Cycle_t cyclesOff = time - last_time - 1;
    // Update busy values
    for ( int i = 0; i < local_port_start + local_ports; ++i) {
        port_busy[i] = (port_busy[i] < cyclesOff) ? 0 : port_busy[i] - cyclesOff;
    }

    // unsigned int local_progress = (cyclesOff * local_lru.size()) % (local_lru.size() * 2);
    // unsigned int mesh_progress = (cyclesOff * mesh_lru.size()) % (mesh_lru.size() * 2);
    // // Update lru info
    // for (unsigned int i = 0; i < local_progress; i++)
    //     local_lru.satisfied(false);

    // for (unsigned int i = 0; i < mesh_progress; i++)
    //     mesh_lru.satisfied(false);
    clock_is_off = false;
}

bool
noc_mesh::clock_handler(Cycle_t cycle)
{
    last_time = cycle;
    // TraceFunction trace(CALL_INFO);
    // Decrement all the busy values
    for ( int i = 0; i < local_port_start + local_ports; ++i ) {
        port_busy[i]--;
        if (port_busy[i] < 0) port_busy[i] = 0;
    }

    bool keepClockOn = false;
    // Progress all the messages


    // Prioirty goes in order of the lru_units list.  First entry has
    // highest priority, second has second highest, etc
    for ( auto& lru : lru_units ) {
        for ( unsigned int i = 0; i < lru.size(); i++ ) {
            int lru_port = lru.top();
            if ( !port_queues[lru_port].empty() ) {
                // noc_mesh_event* event = port_queues[local_port_start + i].front();
                noc_mesh_event* event = port_queues[lru_port].front();
                
                // Get the next port
                int port = event->next_port;

                // Check to see if the port is busy
                if ( port_busy[port] > 0 ) {
                    xbar_stalls[port]->addData(1);
                    lru.satisfied(false);
                    keepClockOn = true;
                    continue;
                }
            
                // Check to see if there are enough credits to send on
                // that port
                // output.output("(%d,%d): clock_handler(): port_credits[%d] = %d\n",my_x,my_y,port,port_credits[port]);
                if ( port_credits[port] >= event->encap_ev->getSizeInFlits() ) {
                    int trace_id = event->encap_ev->request->getTraceID();
                    int vn = event->encap_ev->vn;
                    SST::Interfaces::SimpleNetwork::nid_t src = event->encap_ev->request->src;
                    SST::Interfaces::SimpleNetwork::nid_t dest = event->encap_ev->request->dest;
                    SST::Interfaces::SimpleNetwork::Request::TraceType ttype = event->encap_ev->request->getTraceType();
                    int flits = event->encap_ev->getSizeInFlits();
                    
                    // port_queues[local_port_start + i].pop();
                    port_queues[lru_port].pop();
                    port_credits[port] -= event->encap_ev->getSizeInFlits();
                    port_busy[port] = event->encap_ev->getSizeInFlits();
                    if ( edge_status & ( 1 << port) ) {
                        ports[port]->send(event->encap_ev);
                        send_bit_count[port]->addData(event->encap_ev->request->size_in_bits);
                        event->encap_ev = NULL;
                        delete event;
                    }
                    else {
                        ports[port]->send(event);
                        send_bit_count[port]->addData(event->encap_ev->request->size_in_bits);
                    }
                    if ( ttype == SimpleNetwork::Request::FULL ) {
                        output.output("TRACE(%d): %" PRIu64 " ns: Sent an event to router from router: (%d,%d)"
                                      " (%s) on VC %d from src %" PRIu64 " to dest %" PRIu64 ".\n",
                                      trace_id,
                                      getCurrentSimTimeNano(),
                                      my_x, my_y,
                                      getName().c_str(),
                                      vn,
                                      src,
                                      dest);
                    }
                    // Need to send credit event back to last router
                    credit_event* cr_ev = new credit_event(0, flits);
                    // ports[local_port_start + i]->send(cr_ev);
                    ports[lru_port]->send(cr_ev);
                    lru.satisfied(true);
                }
                else {
                    output_port_stalls[port]->addData(1);
                    lru.satisfied(false);
                }
                if (!port_queues[lru_port].empty())
                    keepClockOn = true;
            }
            else {
                lru.satisfied(false);
            }
        }
    }
    
    // }
    clock_is_off = !keepClockOn;

    // Stay on clock list
    return !keepClockOn;
}

void noc_mesh::setup()
{
    // if ( use_dense_map ) {
    //     if ( my_x == 1 && my_y == 1 ) {
    //         for ( int i = 0; i < total_endpoints; ++i ) {
    //             output.output("%d - %d\n",i,dense_map[i]);
    //         }
    //     }
    // }

    // Set up the lru units

    // First do the endpoints
    lru_units.resize(1);
    for ( int i = local_port_start; i < local_port_start + local_ports; ++i ) {
        if ( ports[i] != NULL ) {
            lru_units[0].insert(i);
        }
    }
    

    // If the priorities aren't equal, create another lru_unit for the
    // lower priority ports
    if ( !port_priority_equal ) {
        lru_units[0].finalize();
        lru_units.resize(2);        
    }
    
    // Now the mesh ports
    for ( int i = 0; i < local_port_start; ++i ) {
        if ( ports[i] != NULL ) {
            lru_units.back().insert(i);
        }
    }
    lru_units.back().finalize();
}

void noc_mesh::finish()
{
}

void
noc_mesh::init(unsigned int phase)
{
    // output.output("%s::init(%u) - init_state = %d\n",getName().c_str(),phase,init_state);
    
    // Init states:
    // 0 - wait for endpoint messages
    //
    // 1 - recv messages from endpoints.  Compute locations in mesh
    // and routers on eastern frontier send west.  Also pass flit_size
    // to endpoints.
    //
    // 2 - Pass messages west, incrementing counter
    //
    // 3 - Western edge waits for endpoint count and x_size (southern
    // row only)
    //
    // 4 - Western edge passes endpoint count and y_size from north
    // port to south port.
    //
    // 5 - Southwestern router (router 0) waits for endpoint count and
    // y_size
    //
    // 6 - Western edge waits for broadcated information from router 0
    //
    // 7 - Non-western edge wait for information broadcast from the
    // west
    //
    // 8 - Send information to endpoints.
    //
    // 9 - Send all credit events

    // Each router needs two variables dealing with endpoint counts:
    //   -  total_endpoints: total number of endpoints
    //   -  endpoint_start: number of endpoints in routers prior to
    //        current router
    // 
    // There are also two intermediate counts: row_endpoints and
    // my_endpoints.  These will be stored in the final values as
    // follows:
    //   - row_endpoints stored in total_endpoints
    //   - my_endpoints stored in endpoint_start
       
    NocInitEvent* nie;
    Event* ev;
    switch ( init_state ) {
    case 0:
        // Phase 0 is only for endpoints to send a message
        init_state = 1;
        break;
    case 1:
    {
        // Look through all the links to see which have endpoints
        // attached or have no links attached
        for ( int i = 0; i < local_port_start + local_ports; ++i ) {
            if ( ports[i] == NULL ) {
                edge_status |=  ( 1 << i );
            }
            else {
                ev = ports[i]->recvInitData();
                if ( ev != NULL ) {
                    nie = static_cast<NocInitEvent*>(ev);
                    if ( nie->command == NocInitEvent::REPORT_ENDPOINT ) {
                        edge_status |=  ( 1 << i );
                        endpoint_locations |= ( 1 << i );
                        endpoint_start++;
                        delete nie;
                        // Endpoint to router link
                        ports[i]->setFunctor(new Event::Handler<noc_mesh,int>(this,&noc_mesh::handle_input_ep2r,i));
                    }
                    else {
                        // Unexpected command
                    }
                }
                else {
                    // Router to router link
                    ports[i]->setFunctor(new Event::Handler<noc_mesh,int>(this,&noc_mesh::handle_input_r2r,i));
                }
            }
        }

        // output.output("%s::init(%u) - edge_status = 0x%x\n",getName().c_str(),phase,edge_status);
        // output.output("%s::init(%u) - endpoint_locations = 0x%x\n",getName().c_str(),phase,endpoint_locations);
        // output.output("my endpoint count = %d\n\n",endpoint_start);

        // Now, all the routers on the eastern frontier send the count
        // of endpoints to the west.  In addition, the southern row
        // will also send the count of columns to get the x_size.
        if ( edge_status & east_mask ) { // East port is an edge
            nie = new NocInitEvent();
            nie->command = NocInitEvent::SUM_ENDPOINTS;
            nie->int_value = endpoint_start;
            ports[west_port]->sendInitData(nie);
            if ( edge_status & south_mask ) { // southeast corner
                nie = new NocInitEvent();
                nie->command = NocInitEvent::COMPUTE_X_SIZE;
                nie->int_value = 1;
                ports[west_port]->sendInitData(nie);
                init_state = 7;
            }
            else {
                init_state = 7;
            }
        }

        else if ( edge_status & 0x8 ) {
            // Western edge waits for endpoint count and x_size
            // computation
            init_state = 3;
        }
        else {
            // All other routers wait to pass endpoint count and
            // x_size
            init_state = 2;
        }

        // Pass flit size to the endpoints
        for ( int i = 0; i < local_port_start + local_ports; ++i ) {
            if ( (1 << i) & endpoint_locations ) {
                nie = new NocInitEvent();
                nie->command = NocInitEvent::REPORT_FLIT_SIZE;
                nie->ua_value = UnitAlgebra("1b") * flit_size;
                ports[i]->sendInitData(nie);
            }
        }
        break;
    }
    case 2:
        // Look for events coming in on east link and increment and
        // pass to west link
        ev = ports[east_port]->recvInitData();
        if ( NULL == ev ) break;
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::SUM_ENDPOINTS ) {
            nie->int_value += endpoint_start;
            ports[west_port]->sendInitData(nie);
            if ( edge_status & south_mask ) { // southern row also computing x_size
                ev = ports[east_port]->recvInitData();
                nie = static_cast<NocInitEvent*>(ev);
                if ( nie->command == NocInitEvent::COMPUTE_X_SIZE ) {
                    nie->int_value++;
                    ports[west_port]->sendInitData(nie);
                }
                else {
                    // unexpected event type
                }
            }
        }
        else {
            // Unexpected event type
        }

        init_state = 7;
        break;
    case 3:
        // Western edge waiting for endpoint count and x_size
        // (southern corner only)
        ev = ports[east_port]->recvInitData();
        if ( NULL == ev ) break;

        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::SUM_ENDPOINTS ) {
            // Compute final row_endpoints value for the row and store
            // in total_endpoints
            total_endpoints = endpoint_start + nie->int_value;
            delete nie;
        }
        else {
            // Unexpected event type
        }
        init_state = 4;
        
        // If south western router (router 0), compute x_size
        if ( edge_status & south_mask ) {
            ev = ports[east_port]->recvInitData();
            if ( NULL == ev ) {} // should have had an event
            nie = static_cast<NocInitEvent*>(ev);
            if ( nie->command == NocInitEvent::COMPUTE_X_SIZE ) {
                // Add one for me, plus the halo (+2)
                x_size = nie->int_value + 3;
                delete nie;
            }
            else {
                // Unexpected event type
            }
            init_state = 5;
        }
        else if ( edge_status & north_mask ) { // northwest corner
            // Send the total endpoint count and y_size count south.
            nie = new NocInitEvent();
            nie->command = NocInitEvent::SUM_ENDPOINTS;
            nie->int_value = total_endpoints;
            ports[south_port]->sendInitData(nie);

            nie = new NocInitEvent();
            nie->command = NocInitEvent::COMPUTE_Y_SIZE;
            nie->int_value = 1;
            ports[south_port]->sendInitData(nie);
            init_state = 6;
        }
        break;
    case 4:
        // Read from north port, increment and send to south port
        ev = ports[north_port]->recvInitData();
        if ( NULL == ev ) break;
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::SUM_ENDPOINTS ) {
            nie->int_value += total_endpoints;
            ports[south_port]->sendInitData(nie);
        }
        else {
            // Unexpected event type
        }

        ev = ports[north_port]->recvInitData();
        if ( NULL == ev ) {} // should be an event
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::COMPUTE_Y_SIZE ) {
            nie->int_value++;
            ports[south_port]->sendInitData(nie);
        }
        else {
            // Unexpected event type
        }
        init_state = 6;
        break;
    case 5:
    {
        // Wait to get endpoint count and y_size

        // Temporarily need to hold row_endpoints
        int row_endpoints = total_endpoints;
        ev = ports[north_port]->recvInitData();
        if ( NULL == ev ) break;
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::SUM_ENDPOINTS ) {
            total_endpoints = row_endpoints + nie->int_value;
            delete nie;
        }
        else {
            // Unexpected event type
        }

        ev = ports[north_port]->recvInitData();
        if ( NULL == ev ) {} // should have been an event
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::COMPUTE_Y_SIZE ) {
            // Add one for me, plus the halo (+2)
            y_size = nie->int_value + 3;
            delete nie;
        }
        else {
            // Unexpected event type
        }

        // Set x and y, remember that there is a virtual halo
        my_x = 1;
        my_y = 1;
        
        // Send the info east and north
        nie = new NocInitEvent();
        nie->command = NocInitEvent::COMPUTE_ENDPOINT_START;
        nie->int_value = row_endpoints;
        ports[north_port]->sendInitData(nie);

        nie = new NocInitEvent();
        nie->command = NocInitEvent::COMPUTE_ENDPOINT_START;
        nie->int_value = endpoint_start;
        endpoint_start = 0;
        ports[east_port]->sendInitData(nie);

        nie = new NocInitEvent();
        nie->command = NocInitEvent::BROADCAST_TOTAL_ENDPOINTS;
        nie->int_value = total_endpoints;
        ports[east_port]->sendInitData(nie->clone());
        ports[north_port]->sendInitData(nie);

        nie = new NocInitEvent();
        nie->command = NocInitEvent::BROADCAST_X_SIZE;
        nie->int_value = x_size;
        ports[east_port]->sendInitData(nie->clone());
        ports[north_port]->sendInitData(nie);

        nie = new NocInitEvent();
        nie->command = NocInitEvent::BROADCAST_Y_SIZE;
        nie->int_value = y_size;
        ports[east_port]->sendInitData(nie->clone());
        ports[north_port]->sendInitData(nie);

        nie = new NocInitEvent();
        nie->command = NocInitEvent::COMPUTE_X_POS;
        nie->int_value = my_x;
        ports[east_port]->sendInitData(nie->clone());
        ports[north_port]->sendInitData(nie);

        nie = new NocInitEvent();
        nie->command = NocInitEvent::COMPUTE_Y_POS;
        nie->int_value = my_y;
        ports[east_port]->sendInitData(nie->clone());
        ports[north_port]->sendInitData(nie);
        
        init_state = 8;
        init_count = (x_size - 2 - my_x) + (y_size - 2 - my_y) - 1;
        break;
    }
    case 6:
        ev = ports[south_port]->recvInitData();
        if ( NULL == ev ) break;
        
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::COMPUTE_ENDPOINT_START ) {
            
            // Make a copy of my_endpoints (stored in endpoint_start)
            // because we need to put the actual endpoint_start in
            // that variable.
            int my_ep = endpoint_start;
            endpoint_start = nie->int_value;

            if ( !( edge_status & north_mask) ) {
                NocInitEvent* nie2 = nie->clone();
                // Add the row endpoints (stored in total_endpoints to
                // what we got and send north
                nie2->int_value += total_endpoints;
                ports[north_port]->sendInitData(nie2);
            }
            nie->int_value += my_ep;
            ports[east_port]->sendInitData(nie);                        
        }

        ev = ports[south_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        // Get all the info and do the appropriate things with it
        if ( nie->command == NocInitEvent::BROADCAST_TOTAL_ENDPOINTS ) {
            total_endpoints = nie->int_value;
            if ( !( edge_status & north_mask) ) ports[north_port]->sendInitData(nie->clone());
            ports[east_port]->sendInitData(nie);
        }

        ev = ports[south_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::BROADCAST_X_SIZE ) {
            x_size = nie->int_value;
            if ( !( edge_status & north_mask) ) ports[north_port]->sendInitData(nie->clone());
            ports[east_port]->sendInitData(nie);
        }

        ev = ports[south_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::BROADCAST_Y_SIZE ) {
            y_size = nie->int_value;
            if ( !( edge_status & north_mask) ) ports[north_port]->sendInitData(nie->clone());
            ports[east_port]->sendInitData(nie);
        }

        ev = ports[south_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::COMPUTE_X_POS ) {
            my_x = nie->int_value;
            if ( !( edge_status & north_mask) ) ports[north_port]->sendInitData(nie->clone());
            ports[east_port]->sendInitData(nie);
        }

        ev = ports[south_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::COMPUTE_Y_POS ) {
            my_y = nie->int_value + 1;
            nie->int_value++;
            if ( !( edge_status & north_mask) ) ports[north_port]->sendInitData(nie->clone());
            ports[east_port]->sendInitData(nie);
        }

        init_count = (x_size - 2 - my_x) + (y_size - 2 - my_y) - 1;
        init_state = 8;
        break;

    case 7:
        ev = ports[west_port]->recvInitData();
        if ( NULL == ev ) break;

        nie = static_cast<NocInitEvent*>(ev);
        // Get all the info and do the appropriate things with it
        if ( nie->command == NocInitEvent::COMPUTE_ENDPOINT_START ) {
            // Make a copy of my_endpoints (stored in endpoint_start)
            // because we need to put the actual endpoint_start in
            // that variable.
            int my_ep = endpoint_start;
            endpoint_start = nie->int_value;

            nie->int_value += my_ep;
            if ( !( edge_status & east_mask) ) ports[east_port]->sendInitData(nie);
            else delete nie;
        }

        ev = ports[west_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::BROADCAST_TOTAL_ENDPOINTS ) {
            total_endpoints = nie->int_value;
            if ( !( edge_status & east_mask) ) ports[east_port]->sendInitData(nie);
            else delete nie;
        }

        ev = ports[west_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::BROADCAST_X_SIZE ) {
            x_size = nie->int_value;
            if ( !( edge_status & east_mask) ) ports[east_port]->sendInitData(nie);
            else delete nie;
        }

        ev = ports[west_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::BROADCAST_Y_SIZE ) {
            y_size = nie->int_value;
            if ( !( edge_status & east_mask) ) ports[east_port]->sendInitData(nie);
            else delete nie;
        }

        ev = ports[west_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::COMPUTE_X_POS ) {
            my_x = nie->int_value + 1;
            nie->int_value++;
            if ( !( edge_status & east_mask) ) ports[east_port]->sendInitData(nie);
            else delete nie;
        }

        ev = ports[west_port]->recvInitData();
        nie = static_cast<NocInitEvent*>(ev);
        if ( nie->command == NocInitEvent::COMPUTE_Y_POS ) {
            my_y = nie->int_value;
            if ( !( edge_status & east_mask) ) ports[east_port]->sendInitData(nie);
            else delete nie;
        }

        init_state = 8;
        if ( !((edge_status & north_mask) && (edge_status & east_mask)) ) {
            // All but the northeast corner (the last router to get
            // the information broadcast) go to state 8 with a counter
            // to tell it when the northeast corner hits that state.
            // Then all routers will populate the dense_map and send
            // info to the endpoints.
            init_count = (x_size - 2 - my_x) + (y_size - 2 - my_y) - 1;
            break;
        }
        //break;
    case 8:
    {
        if ( init_count > 0 ) {
            init_count--;
            break;
        }
        // Used if we are setting up the dense to sparse id map
        std::vector<std::pair<int,int>> ep_ids;
        // Need to send the information to the endpoints
        // See if any of the directional ports have endpoints
        if ( endpoint_locations & north_mask ) {
            int x = my_x;
            int y = my_y + 1;
            int endpoint_id = ((y * x_size) + x) * local_ports;
            // This is odd, but a static const int does not allocate
            // space, so need to put it in a temp variable because
            // make_pair is pass by reference.
            int tmp = north_port;
            ep_ids.push_back(std::make_pair(endpoint_id,tmp));
        }
        
        if ( endpoint_locations & south_mask ) {
            int x = my_x;
            int y = my_y - 1;
            int endpoint_id = ((y * x_size) + x) * local_ports;
            // This is odd, but a static const int does not allocate
            // space, so need to put it in a temp variable because
            // make_pair is pass by reference.
            int tmp = south_port;
            ep_ids.push_back(std::make_pair(endpoint_id,tmp));
        }
        
        if ( endpoint_locations & east_mask ) {
            int x = my_x + 1;
            int y = my_y;
            int endpoint_id = ((y * x_size) + x) * local_ports;
            // This is odd, but a static const int does not allocate
            // space, so need to put it in a temp variable because
            // make_pair is pass by reference.
            int tmp = east_port;
            ep_ids.push_back(std::make_pair(endpoint_id,tmp));
        }
        
        if ( endpoint_locations & west_mask ) {
            int x = my_x - 1;
            int y = my_y;
            int endpoint_id = ((y * x_size) + x) * local_ports;
            // This is odd, but a static const int does not allocate
            // space, so need to put it in a temp variable because
            // make_pair is pass by reference.
            int tmp = west_port;
            ep_ids.push_back(std::make_pair(endpoint_id,tmp));
        }

        // Now for local ports
        for ( int i = 0; i < local_ports; ++i ) {
            if ( endpoint_locations & ( 1 << (i + local_port_start) ) ) {
                int endpoint_id = (((my_y * x_size) + my_x) * local_ports) + i;
                ep_ids.push_back(std::make_pair(endpoint_id,local_port_start + i));
            }
        }

        init_state = 9;
        // output.output("Router %s has x = %d and y = %d\n",getName().c_str(),my_x,my_y);
        // output.output("Total endpoints = %d\n",total_endpoints);
        // output.output("Endpoint_Start = %d\n",endpoint_start);
        // output.output("x_size = %d, y_size = %d\n",x_size, y_size);

        // Create a shared region that gives a dense to sparse mapping
        // for network addresses.  This is only used if the
        // use_dense_map param is set to true
        if ( use_dense_map ) {
            SharedRegion* sr = Simulation::getSharedRegionManager()->getGlobalSharedRegion(
                "noc_mesh_dense_map",
                16 * total_endpoints * sizeof(int),
                new SharedRegionMerger());

            std::sort(ep_ids.begin(), ep_ids.end());
            
            for ( int i = 0; i < ep_ids.size(); ++i ) {
                sr->modifyArray<int>(endpoint_start + i, ep_ids[i].first);
                ep_ids[i].first = endpoint_start + i;
            }
            sr->publish();
            dense_map = sr->getPtr<const int*>();
        }

        // Send all the endpoint notifications
        for ( auto i : ep_ids ) {
            nie = new NocInitEvent();
            nie->command = NocInitEvent::REPORT_ENDPOINT_ID;
            nie->int_value = i.first;
            ports[i.second]->sendInitData(nie);
        }
        break;
    }
    case 9:
    {
        
        for ( int i = 0; i < local_port_start + local_ports; ++i ) {
            if ( ports[i] != NULL ) {
                credit_event* cr_ev = new credit_event(0,input_buf_size/flit_size);
                ports[i]->sendInitData(cr_ev);
            }
        }

        init_state = 10;
        break;
    }
    case 10:
    {
        // Receive credits
        for ( int i = 0; i < local_port_start + local_ports; ++i ) {
            if ( ports[i] != NULL ) {
                credit_event* cr_ev = static_cast<credit_event*>(ports[i]->recvInitData());
                port_credits[i] += cr_ev->credits;
            }
        }
        init_state = 11;
        // Falls through on purpose
    }
    default:
        // Simply route messages that are sent by the endpoints
        for ( int i = 0; i < local_port_start + local_ports; ++i ) {
            if ( ports[i] != NULL ) {
                bool endpoint = (1 << i) & endpoint_locations;
                while ( true ) { // Go until there are no more events
                    Event* ev = ports[i]->recvInitData();
                    if ( NULL == ev ) break;

                    noc_mesh_event* nme;
                    if ( endpoint ) {
                        NocPacket* packet = static_cast<NocPacket*>(ev);
                        nme = wrap_incoming_packet(packet);
                        
                    }
                    else {
                        nme = static_cast<noc_mesh_event*>(ev);
                    }
                    
                    // Route the packet, but look for broadcasts first
                    if ( nme->egress_port == -1 ) {
                        // This is a broadcast, need to decide which
                        // phase we are in:
                        //
                        // endpoint: just came from endpoint, need to
                        // send it all 4 directions.
                        //
                        // east/west: if it came from east or west,
                        // send to opposite direction and north and
                        // south.
                        //
                        // north/south: if it came from north or
                        // south, just send in opposite direction.
                        //
                        // All of these case will deliver to all the
                        // endpoints (except that we don't send it to
                        // the endpoint we just got it from if we are
                        // in that phase).

                        // Send east.  We send east if this came from
                        // an endpoint or from the west.
                        if ( endpoint || ( (1 << i ) & west_mask ) ) {
                            // No need to send east if this is the
                            // eastern edge
                            if ( !(edge_status & east_mask) ) {
                                ports[east_port]->sendInitData(nme->clone());
                            }
                        }
                        
                        // Send west.  We send west if this came from
                        // an endpoint or from the east.
                        if ( endpoint || ( (1 << i ) & east_mask ) ) {
                            // No need to send east if this is the
                            // eastern edge
                            if ( !(edge_status & west_mask) ) {
                                ports[west_port]->sendInitData(nme->clone());
                            }
                        }
                        
                        // Send north.  We send north if this came
                        // from an endpoint, or from the east, west or
                        // south.
                        if ( endpoint || ( (1 << i ) & west_mask ) ||
                            ( (1 << i ) & east_mask ) || ( (1 << i ) & south_mask )) {
                            // No need to send north if this is the
                            // northern edge
                            if ( !(edge_status & north_mask) ) {
                                ports[north_port]->sendInitData(nme->clone());
                            }
                        }
                        
                        // Send south.  We send south if this came
                        // from an endpoint, or from the east, west or
                        // north.
                        if ( endpoint || ( (1 << i ) & west_mask ) ||
                            ( (1 << i ) & east_mask ) || ( (1 << i ) & north_mask )) {
                            // No need to send south if this is the
                            // southern edge
                            if ( !(edge_status & south_mask) ) {
                                ports[south_port]->sendInitData(nme->clone());
                            }
                        }

                        // Now send to all the endpoints
                        bool sent = false;
                        NocPacket* packet = nme->encap_ev;
                        nme->encap_ev = NULL;
                        delete nme;
                        for ( int j = 0; j < local_port_start + local_ports; ++j ) {
                            if ( endpoint && ( i == j ) ) continue;  // No need to send back to src
                            if ( (1 << j) & endpoint_locations ) {
                                if (!sent) {
                                    ports[j]->sendInitData(packet);
                                    sent = true;
                                }
                                else {
                                    ports[j]->sendInitData(packet->clone());
                                }
                            }
                        }
                        if ( !sent ) delete packet;
                        
                    }
                    else { // Not a broadcast
                        route(nme);
                        if ( (1 << nme->next_port) & endpoint_locations ) {
                            ports[nme->next_port]->sendInitData(nme->encap_ev);
                            nme->encap_ev = NULL;
                            delete nme;
                        }
                        else {
                            ports[nme->next_port]->sendInitData(nme);
                        }       
                    }
                }
            }
        }
        
        break;
    }

    return;


}

void
noc_mesh::complete(unsigned int phase)
{
    // Simply route messages that are sent by the endpoints
    for ( int i = 0; i < local_port_start + local_ports; ++i ) {
        if ( ports[i] != NULL ) {
            bool endpoint = (1 << i) & endpoint_locations;
            while ( true ) { // Go until there are no more events
                Event* ev = ports[i]->recvInitData();
                if ( NULL == ev ) break;
                
                noc_mesh_event* nme;
                if ( endpoint ) {
                    NocPacket* packet = static_cast<NocPacket*>(ev);
                    nme = wrap_incoming_packet(packet);
                    
                }
                else {
                    nme = static_cast<noc_mesh_event*>(ev);
                }
                
                // Route the packet, but look for broadcasts first
                if ( nme->egress_port == -1 ) {
                    // This is a broadcast, need to decide which
                    // phase we are in:
                    //
                    // endpoint: just came from endpoint, need to
                    // send it all 4 directions.
                    //
                    // east/west: if it came from east or west,
                    // send to opposite direction and north and
                    // south.
                    //
                    // north/south: if it came from north or
                    // south, just send in opposite direction.
                    //
                    // All of these case will deliver to all the
                    // endpoints (except that we don't send it to
                    // the endpoint we just got it from if we are
                    // in that phase).
                    
                    // Send east.  We send east if this came from
                    // an endpoint or from the west.
                    if ( endpoint || ( (1 << i ) & west_mask ) ) {
                        // No need to send east if this is the
                        // eastern edge
                        if ( !(edge_status & east_mask) ) {
                            ports[east_port]->sendInitData(nme->clone());
                        }
                    }
                    
                    // Send west.  We send west if this came from
                    // an endpoint or from the east.
                    if ( endpoint || ( (1 << i ) & east_mask ) ) {
                        // No need to send east if this is the
                        // eastern edge
                        if ( !(edge_status & west_mask) ) {
                            ports[west_port]->sendInitData(nme->clone());
                        }
                    }
                    
                    // Send north.  We send north if this came
                    // from an endpoint, or from the east, west or
                    // south.
                    if ( endpoint || ( (1 << i ) & west_mask ) ||
                         ( (1 << i ) & east_mask ) || ( (1 << i ) & south_mask )) {
                        // No need to send north if this is the
                        // northern edge
                        if ( !(edge_status & north_mask) ) {
                            ports[north_port]->sendInitData(nme->clone());
                        }
                    }
                    
                    // Send south.  We send south if this came
                    // from an endpoint, or from the east, west or
                    // north.
                    if ( endpoint || ( (1 << i ) & west_mask ) ||
                         ( (1 << i ) & east_mask ) || ( (1 << i ) & north_mask )) {
                        // No need to send south if this is the
                        // southern edge
                        if ( !(edge_status & south_mask) ) {
                            ports[south_port]->sendInitData(nme->clone());
                        }
                    }
                    
                    // Now send to all the endpoints
                    bool sent = false;
                    NocPacket* packet = nme->encap_ev;
                    nme->encap_ev = NULL;
                    delete nme;
                    for ( int j = 0; j < local_port_start + local_ports; ++j ) {
                        if ( endpoint && ( i == j ) ) continue;  // No need to send back to src
                        if ( (1 << j) & endpoint_locations ) {
                            if (!sent) {
                                ports[j]->sendInitData(packet);
                                sent = true;
                            }
                            else {
                                ports[j]->sendInitData(packet->clone());
                            }
                        }
                    }
                    if ( !sent ) delete packet;
                    
                }
                else { // Not a broadcast
                    route(nme);
                    if ( (1 << nme->next_port) & endpoint_locations ) {
                        ports[nme->next_port]->sendInitData(nme->encap_ev);
                        nme->encap_ev = NULL;
                        delete nme;
                    }
                    else {
                        ports[nme->next_port]->sendInitData(nme);
                    }       
                }
            }
        }
    }
    
    return;

}


void
noc_mesh::printStatus(Output& out)
{
    out.output("Start Router %s:  id = (%d, %d)\n", getName().c_str(), my_x, my_y);

    std::vector<std::pair<std::string,int> > vec;
    int port = north_port;
    vec.push_back(std::make_pair("North", port));
    port = south_port;
    vec.push_back(std::make_pair("South", port));
    port = east_port;
    vec.push_back(std::make_pair("East", port));
    port = west_port;
    vec.push_back(std::make_pair("West", port));

    for ( int i = 0; i < local_ports; ++i ) {
        std::string str = "local_port" + std::to_string(i);
        vec.push_back(std::make_pair(str,i+local_port_start));
    }

    // Add local ports

    for ( auto& pinfo : vec ) {
        out.output("  %s port:\n", pinfo.first.c_str());
        if ( ports[pinfo.second] != NULL ) {
            out.output("    Port busy = %d\n",port_busy[pinfo.second]);
            out.output("    Port credits = %d\n",port_credits[pinfo.second]);
            out.output("    Input queue total packets = %lu, head packet info:\n",port_queues[pinfo.second].size());
            if ( port_queues[pinfo.second].empty() ) {
                out.output("      <empty>\n");
            }
            else {
                noc_mesh_event* event = port_queues[pinfo.second].front();
                out.output("      src = %lld, dest = %lld, next_port = %d, flits = %d\n",
                           event->encap_ev->request->src, event->encap_ev->request->dest,
                           event->next_port, event->encap_ev->getSizeInFlits());
            }
        }
        else {
            out.output("    UNUSED\n");
        }
    }
    
    out.output("End Router %s: id = (%d, %d)\n\n", getName().c_str(), my_x, my_y);
}

