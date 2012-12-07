
#ifndef _MESSAGECOMPONENT_H
#define _MESSAGECOMPONENT_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <cstring>
#include <string>

class messageGeneratorComponent : public SST::Component {
public:

  messageGeneratorComponent(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup()  { return 0; }
  int Finish() { return 0; }

private:
  messageGeneratorComponent();  // for serialization only
  messageGeneratorComponent(const messageGeneratorComponent&); // do not implement
  void operator=(const messageGeneratorComponent&); // do not implement

  void handleEvent( SST::Event *ev );
  virtual bool tick( SST::Cycle_t );

  std::string clock_frequency_str;
  int    message_counter_sent;
  int	 message_counter_recv;
  int    total_message_send_count;
  int	 output_message_info;

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

    remote_component->setFunctor(new SST::Event::Handler<messageGeneratorComponent>(this,&messageGeneratorComponent::handleEvent));
  }
    
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

#endif /* _MESSAGECOMPONENT_H */
