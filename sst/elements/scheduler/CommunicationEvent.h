/*
 * Used for communication between components, including passing instructions, requesting information, and communicating state changes.
 */

#ifndef __COMMUNICATIONEVENT_H__
#define __COMMUNICATIONEVENT_H__

#include <string>

namespace SST {
namespace Scheduler {

enum CommunicationTypes { RETRIEVE_ID, START_FAULTING, LOG_JOB_START, UNREGISTER_YOURSELF, START_FILE_WATCH, START_NEXT_JOB };

class CommunicationEvent : public SST::Event{
  public:

    CommunicationEvent( enum CommunicationTypes type ) : SST::Event(){
      CommType = type;
      reply = false;
    }

    CommunicationEvent( enum CommunicationTypes type, void * payload ) : SST::Event(){
      CommType = type;
      reply = false;
      this->payload = payload;
    }

    enum CommunicationTypes CommType;
    bool reply;
    void * payload;
};

}
}
#endif

