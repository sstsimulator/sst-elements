#ifndef _OMNET_EVENT_H
#define _OMNET_EVENT_H

#include <sst/core/event.h>
#include <omnetpp.h>

//class cMessage;

class OmnetEvent : public SST::Event {
 public:
    cMessage* payload;
  
 OmnetEvent() : SST::Event() { payload = NULL; }
  
 private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(payload);
    }
};

#endif
