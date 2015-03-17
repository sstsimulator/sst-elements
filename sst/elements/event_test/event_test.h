// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
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

namespace SST {
namespace EventTest {

class event_test : public SST::Component {
public:
    event_test(SST::ComponentId_t id, SST::Params& params);

    void init(unsigned int phase);
    void setup();
    void finish();

private:
    event_test();  // for serialization only
    event_test(const event_test&); // do not implement
    void operator=(const event_test&); // do not implement

    void handleEvent( SST::Event *ev );

    int my_id;
    int count_to;
    int latency;
    bool done;
    SST::Link* link;

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(my_id);
        ar & BOOST_SERIALIZATION_NVP(count_to);
        ar & BOOST_SERIALIZATION_NVP(latency);
        ar & BOOST_SERIALIZATION_NVP(done);
        ar & BOOST_SERIALIZATION_NVP(link);
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(my_id);
        ar & BOOST_SERIALIZATION_NVP(count_to);
        ar & BOOST_SERIALIZATION_NVP(latency);
        ar & BOOST_SERIALIZATION_NVP(done);
        ar & BOOST_SERIALIZATION_NVP(link);

//         SST::EventHandler_t* linkHandler = new SST::EventHandler<event_test,bool,SST::Event*>
//             (this,&event_test::handleEvent);
//         link->setFunctor(linkHandler);
	link->setFunctor(new SST::Event::Handler<event_test>(this,&event_test::handleEvent));
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

}
}

#endif
