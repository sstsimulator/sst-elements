// -*- mode: c++ -*-

// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_TEST_BISECTION_BISECTION_TEST_H
#define COMPONENTS_MERLIN_TEST_BISECTION_BISECTION_TEST_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/simpleNetwork.h>


namespace SST {
namespace Merlin {


class bisection_test : public Component {

private:
    int id;
    int partner_id;
    int num_vns;
    int num_peers;
    
    int packets_sent;
    int packets_recd;

    SimTime_t start_time;
    
    int packets_to_send;
    int packet_size;
    UnitAlgebra buffer_size;
    
    SST::Interfaces::SimpleNetwork* link_control;
    Link* self_link;

public:
    bisection_test(ComponentId_t cid, Params& params);
    ~bisection_test() {}

    void init(unsigned int phase);
    void setup(); 
    void finish();


private:
    bool clock_handler(Cycle_t cycle);
    void handle_complete(Event* ev);
    bool receive_handler(int vn);
    bool send_handler(int vn);
    
};

class bisection_test_event : public Event {

 public:
    SimTime_t start_time;

    bisection_test_event() {}
    
    virtual Event* clone(void)
    {
        return new bisection_test_event(*this);
    }

    void serialize_order(SST::Core::Serialization::serializer &ser) {
        Event::serialize_order(ser);
        ser & start_time;
    }

private:
    ImplementSerializable(SST::Merlin::bisection_test_event)
    
};
} // namespace merlin
} // namespace sst
#endif // COMPONENTS_MERLIN_TEST_NIC_H
