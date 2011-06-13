
#ifndef _nicPtlEvent_h
#define _nicPtlEvent_h

#include <sst/core/event.h>

#include "cmdQueueEntry.h"

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
        cmd = entry->cmd;
        context = entry->context;
      //  printf("%s() cmd=%d\n",__func__,cmd);
        for ( int i = 0; i < NUM_ARGS; i++ ) {
            args[i] = entry->args[i];
     //       printf("%s() arg%d %#lx\n",__func__,i,args[i]);
        }
    }
    int cmd;
    int context;
    unsigned long args[NUM_ARGS];

  private:
        
    PtlNicEvent() {}

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {   
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP( cmd );
        ar & BOOST_SERIALIZATION_NVP( context );
        ar & BOOST_SERIALIZATION_NVP( args );
    }
};

#endif
