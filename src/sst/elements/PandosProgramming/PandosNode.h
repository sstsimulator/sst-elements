#pragma once

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <vector>

namespace SST {
namespace PandosProgramming {

/**
 * PANDOS Node component
 */
class PandosNodeT : public SST::Component {
public:
        /**
         * Constructors/Destructors
         */
        PandosNodeT(ComponentId_t id, Params &params);
        ~PandosNodeT();

        /**
         * Event handler which we register to a link
         */
        void handleEvent(SST::Event *ev);

        /**
         * Clock tick handler
         */
        virtual bool clockTic(SST::Cycle_t);

        // SST Output object, for printing error messages, etc.
        SST::Output *out;

        // Links to other nodes
        std::vector<SST::Link*> links;
};

}
}
