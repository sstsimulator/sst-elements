#ifndef _m5_h
#define _m5_h

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/component.h>

class M5 : public SST::Component
{
    class Event : public SST::Event {
      public:
        Event() { 
            setPriority(99);
        }
        SST::Cycle_t time;
        SST::Cycle_t cycles;
    };

  public:
    M5( SST::ComponentId_t id, Params_t& params );
    ~M5();
    int Setup();
    void catchup( SST::Cycle_t );
    void arm( SST::Cycle_t );

  private:
    void selfEvent( SST::Event* );

    SST::Link*          m_self;
    bool                m_armed;
    Event               m_event;
    SST::TimeConverter *m_tc;
};

#endif
