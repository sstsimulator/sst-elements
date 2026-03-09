#pragma once

#include <sst/elements/ember/emberevent.h>
#include <sst/elements/hermes/networkIOapi.h>

using namespace Hermes;

namespace SST {
namespace Ember {

typedef Statistic<uint32_t> EmberEventTimeStatistic;

class EmberNetworkIOEvent : public EmberEvent {
public:
    EmberNetworkIOEvent(NetworkIO::Interface& api, Output* output, 
                   EmberEventTimeStatistic* stat = NULL) :
        EmberEvent(output, stat), m_api(api)
    {
        m_state = IssueCallback;  
    }

protected:
    NetworkIO::Interface& m_api;
};

}
}

