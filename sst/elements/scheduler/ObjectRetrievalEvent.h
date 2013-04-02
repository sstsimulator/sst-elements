/*
 * Used to request an object via an event.  Useful if more direct communication
 * is required between two Components to organize some meta-simulation event.
 */

#ifndef __OBJECTRETRIEVALEVENT_H__
#define __OBJECTRETRIEVALEVENT_H__

#include <string>

namespace SST {
namespace Scheduler {

class ObjectRetrievalEvent : public SST::Event{
  public:
    ObjectRetrievalEvent() : SST::Event(){}
    ObjectRetrievalEvent * copy(){
      ObjectRetrievalEvent * newEvent = new ObjectRetrievalEvent();
      newEvent->payload = payload;

      return newEvent;
    }

    SST::Component * payload;
};

}
}
#endif

