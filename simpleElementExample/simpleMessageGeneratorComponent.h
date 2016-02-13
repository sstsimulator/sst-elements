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

#ifndef _SIMPLEMESSAGEGENERATORCOMPONENT_H
#define _SIMPLEMESSAGEGENERATORCOMPONENT_H

#include "sst/core/component.h"
#include "sst/core/link.h"

namespace SST {
namespace SimpleMessageGeneratorComponent {

class simpleMessageGeneratorComponent : public SST::Component 
{
public:
    simpleMessageGeneratorComponent(SST::ComponentId_t id, SST::Params& params);
    void setup()  { }
    void finish() 
    {
        fprintf(stdout, "Component completed at: %" PRIu64 " milliseconds\n",
		(uint64_t) getCurrentSimTimeMilli() );
    }

private:
    simpleMessageGeneratorComponent();  // for serialization only
    simpleMessageGeneratorComponent(const simpleMessageGeneratorComponent&); // do not implement
    void operator=(const simpleMessageGeneratorComponent&); // do not implement

    void handleEvent(SST::Event *ev);
    virtual bool tick(SST::Cycle_t);
    
    std::string clock_frequency_str;
    int message_counter_sent;
    int message_counter_recv;
    int total_message_send_count;
    int output_message_info;
    
    SST::Link* remote_component;
    
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(clock_frequency_str);
        ar & BOOST_SERIALIZATION_NVP(message_counter_sent);
        ar & BOOST_SERIALIZATION_NVP(message_counter_recv);
        ar & BOOST_SERIALIZATION_NVP(total_message_send_count);
        ar & BOOST_SERIALIZATION_NVP(remote_component);
        ar & BOOST_SERIALIZATION_NVP(output_message_info);
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version) 
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(clock_frequency_str);
        ar & BOOST_SERIALIZATION_NVP(message_counter_sent);
        ar & BOOST_SERIALIZATION_NVP(message_counter_recv);
        ar & BOOST_SERIALIZATION_NVP(total_message_send_count);
        ar & BOOST_SERIALIZATION_NVP(remote_component);
        ar & BOOST_SERIALIZATION_NVP(output_message_info);
        
        remote_component->setFunctor(new SST::Event::Handler<simpleMessageGeneratorComponent>(this,&simpleMessageGeneratorComponent::handleEvent));
    }
      
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

} // namespace SimpleMessageGeneratorComponent
} // namespace SST

#endif /* _SIMPLEMESSAGEGENERATORCOMPONENT_H */
