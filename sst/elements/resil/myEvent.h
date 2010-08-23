#ifndef _MYEVENT_H
#define _MYEVENT_H

#include <sst/core/compEvent.h>

class MyEvent : public SST::CompEvent {
public:
    MyEvent() : SST::CompEvent() { }
    int duration;  //in number of cycles for a job

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        boost::serialization::base_object<CompEvent>(*this);
        ar & BOOST_SERIALIZATION_NVP(duration);
    }
}; 
    
#endif
