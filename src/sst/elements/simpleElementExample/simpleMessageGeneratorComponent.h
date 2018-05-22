// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLEMESSAGEGENERATORCOMPONENT_H
#define _SIMPLEMESSAGEGENERATORCOMPONENT_H

#include "sst/core/component.h"
#include <sst/core/elementinfo.h>
#include "sst/core/link.h"

namespace SST {
namespace SimpleMessageGeneratorComponent {

class simpleMessageGeneratorComponent : public SST::Component 
{
public:

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        simpleMessageGeneratorComponent,
        "simpleElementExample",
        "simpleMessageGeneratorComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Messaging rate benchmark component",
        COMPONENT_CATEGORY_NETWORK
    )
    
    SST_ELI_DOCUMENT_PARAMS(
        { "printStats", "Prints the statistics from the component", "0"},
        { "clock", "Sets the clock for the message generator", "1GHz" },
        { "sendcount", "Sets the number of sends in the simulation.", "1000" },
        { "outputinfo", "Sets the level of output information", "1" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
        { "remoteComponent", "Sets the link for the message component, message components talk to each other exchanging simple messages", { "simpleMessageGeneratorComponent.simpleMessage", "" } }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    simpleMessageGeneratorComponent(SST::ComponentId_t id, SST::Params& params);
    void setup()  { }
    void finish() 
    {
        fprintf(stdout, "Component completed at: %" PRIu64 " milliseconds\n",
		(uint64_t) getCurrentSimTimeMilli() );
    }

private:
    simpleMessageGeneratorComponent();  // for serialization only
    simpleMessageGeneratorComponent(const simpleMessageGeneratorComponent&); // do not implement
    void operator=(const simpleMessageGeneratorComponent&); // do not implement

    void handleEvent(SST::Event *ev);
    virtual bool tick(SST::Cycle_t);
    
    std::string clock_frequency_str;
    int message_counter_sent;
    int message_counter_recv;
    int total_message_send_count;
    int output_message_info;
    
    SST::Link* remote_component;
    
};

} // namespace SimpleMessageGeneratorComponent
} // namespace SST

#endif /* _SIMPLEMESSAGEGENERATORCOMPONENT_H */
