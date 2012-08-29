/*
 * Used for communication between components, including passing instructions, requesting information, and communicating state changes.
 */

#ifndef __COMMUNICATIONEVENT_H__
#define __COMMUNICATIONEVENT_H__

#include <string>

enum CommunicationTypes { ID, LOG_JOB_START, UNREGISTER_YOURSELF, START_FILE_WATCH };

class CommunicationEvent : public SST::Event{
  public:

    CommunicationEvent( enum CommunicationTypes type ) : SST::Event(){
      CommType = type;
      reply = false;
    }

    enum CommunicationTypes CommType;
    bool reply;
    std::string payload;
};

#endif

