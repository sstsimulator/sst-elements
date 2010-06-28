#ifndef _SSTDISKSIM_EVENT_H
#define _SSTDISKSIM_EVENT_H

#include <sst/core/event.h>

enum eventtype {READ, WRITE};

class sstdisksim_event : public SST::Event {
public:
  eventtype etype;
  int id;
  int addr;
 sstdisksim_event() : SST::Event() { }

private:
  friend class boost::serialization::access;
  template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
      ar & BOOST_SERIALIZATION_NVP(etype);
    }
}; 
    
#endif /* _SSTDISKSIM_EVENT_H */
