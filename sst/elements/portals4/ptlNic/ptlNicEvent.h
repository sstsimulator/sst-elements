#ifndef _nicPtlEvent_h
#define _nicPtlEvent_h

#include <sst/core/event.h>

#include "cmdQueueEntry.h"

namespace SST {
namespace Portals4 {

class PtlNicRespEvent : public SST::Event {
  public:
    PtlNicRespEvent( int _retval ) :
        retval( _retval ) 
    {
       // printf("%s() retval=%#x\n",__func__,retval);
    }
    int retval;

  private:
    PtlNicRespEvent() {}
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {   
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP( retval );
    }
};

class PtlNicEvent : public SST::Event {

  public:
    PtlNicEvent( cmdQueueEntry_t* entry )
    { 
        memcpy( data, entry, sizeof( *entry ) );
    }

    cmdQueueEntry_t& cmd() { 
        return *((cmdQueueEntry_t*) data );
    }

  private:
    PtlNicEvent() {}
        
    unsigned char data[ sizeof( cmdQueueEntry_t ) ];

    friend class boost::serialization::access;
    template<class Archive>
    void serialize( Archive & ar, const unsigned int version )
    {   
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Event );
        ar & BOOST_SERIALIZATION_NVP( data );
    }
};

}
}

#endif
