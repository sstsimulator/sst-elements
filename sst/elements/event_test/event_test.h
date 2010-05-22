// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _EVENT_TEST_H
#define _EVENT_TEST_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

class event_test : public SST::Component {
public:
    event_test( SST::ComponentId_t id, SST::Component::Params_t& params );

    int Setup();
    int Finish() {
	return 0;
    }
    
private:
    event_test() { }

    int my_id;
    int count_to;
    int latency;
    bool done;

    SST::Link* link;
    
    bool handleEvent( SST::Event *ev );

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        boost::serialization::base_object<Component>(*this);
        ar & BOOST_SERIALIZATION_NVP(my_id);
        ar & BOOST_SERIALIZATION_NVP(count_to);
        ar & BOOST_SERIALIZATION_NVP(latency);
        ar & BOOST_SERIALIZATION_NVP(done);
    }
};

#endif
