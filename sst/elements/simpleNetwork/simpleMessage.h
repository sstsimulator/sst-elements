
#ifndef _SIMPLEMESSAGE_H
#define _SIMPLEMESSAGE_H

#include <sst/core/event.h>

class simpleMessage : public SST::Event {
public:
  simpleMessage() : SST::Event() { }

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
    }
}; 
    
#endif /* _SIMPLEMESSAGE_H */
