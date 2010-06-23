#ifndef _MYEVENT_H
#define _MYEVENT_H

#include <sst/core/event.h>

class MyEvent : public SST::Event {
public:
    MyEvent() : SST::Event() { }
    int count;

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(count);
    }
}; 
    
#endif
