
#ifndef _MESSAGECOMPONENT_H
#define _MESSAGECOMPONENT_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <cstring>

class messageGeneratorComponent : public SST::Component {
public:

  messageGeneratorComponent(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup()  { return 0; }
  int Finish() { return 0; }

private:
  messageGeneratorComponent();  // for serialization only
  messageGeneratorComponent(const simpleComponent&); // do not implement
  void operator=(const simpleComponent&); // do not implement

  void handleEvent( SST::Event *ev );
  virtual bool tick( SST::Cycle_t );

  string clock_frequency_str;
  int    message_counter_sent;
  int	 message_counter_recv;

  SST::Link* remote_component;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(clock_frequency_str);
    ar & BOOST_SERIALIZATION_NVP(message_counter_sent);
    ar & BOOST_SERIALIZATION_NVP(message_counter_recv);
    ar & BOOST_SERIALIZATION_NVP(remote_component);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(clock_frequency_str);
    ar & BOOST_SERIALIZATION_NVP(message_counter_sent);
    ar & BOOST_SERIALIZATION_NVP(message_counter_recv);
    ar & BOOST_SERIALIZATION_NVP(remote_component);

    remote_component->setFunctor(new SST::Event::Handler<messageGeneratorComponent>(this,&messageGeneratorComponent::handleEvent));
  }
    
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

#endif /* _MESSAGECOMPONENT_H */
