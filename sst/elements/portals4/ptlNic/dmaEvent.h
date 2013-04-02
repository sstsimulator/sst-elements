#ifndef _dmaEvent_h
#define _dmaEvent_h

#include <sst/core/event.h>

#include "ptlNicTypes.h"

namespace SST {
namespace Portals4 {

class DmaEvent : public SST::Event {

  public:
    enum Type { Read, Write };

    DmaEvent( Type _type, Addr _addr, uint8_t* _buf,
                size_t _size, void* _key ) :
        type( _type ),
        addr( _addr ),
        buf( _buf ),
        size( _size ), // this is a hack, used by PtlNicMMIF
        key( _key )
    { 
    }
    Type        type; 
    Addr        addr;     
    uint8_t*    buf;
    size_t      size;
    size_t      _size;
    void*       key;
    
  private:
        
    ~DmaEvent() {}

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {   
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(type);
        ar & BOOST_SERIALIZATION_NVP(addr);
        ar & BOOST_SERIALIZATION_NVP(buf);
        ar & BOOST_SERIALIZATION_NVP(size);
    }
};

}
}

#endif
