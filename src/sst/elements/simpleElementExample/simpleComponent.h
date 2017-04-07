// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLECOMPONENT_H
#define _SIMPLECOMPONENT_H

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/link.h>
#include <sst/core/rng/marsaglia.h>

namespace SST {
namespace SimpleComponent {

class simpleComponent : public SST::Component 
{
public:
    simpleComponent(SST::ComponentId_t id, SST::Params& params);
    ~simpleComponent();

    void setup() { }
    void finish() {
    	printf("Component Finished.\n");
    }

private:
    simpleComponent();  // for serialization only
    simpleComponent(const simpleComponent&); // do not implement
    void operator=(const simpleComponent&); // do not implement

    void handleEvent(SST::Event *ev);
    virtual bool clockTic(SST::Cycle_t);

    int workPerCycle;
    int commFreq;
    int commSize;
    int neighbor;

    SST::RNG::MarsagliaRNG* rng;
    SST::Link* N;
    SST::Link* S;
    SST::Link* E;
    SST::Link* W;

//static const ElementInfoParam simpleComponent_params[] = {
//    { "workPerCycle", "Count of busy work to do during a clock tick.", NULL},
//    { "commFreq", "Approximate frequency of sending an event during a clock tick.", NULL},
//    { "commSize", "Size of communication to send.", "16"},
//    { NULL, NULL, NULL}
//};

//static const char* simpleComponent_port_events[] = { "simpleComponent.simpleComponentEvent", NULL };

//static const ElementInfoPort simpleComponent_ports[] = {
//    {"Nlink", "Link to the simpleComponent to the North", simpleComponent_port_events},
//    {"Slink", "Link to the simpleComponent to the South", simpleComponent_port_events},
//    {"Elink", "Link to the simpleComponent to the East",  simpleComponent_port_events},
//    {"Wlink", "Link to the simpleComponent to the West",  simpleComponent_port_events},
//    {NULL, NULL, NULL}
//};

//static const ElementInfoComponent simpleElementComponents[] = {
//    { "simpleComponent",                                 // Name
//      "Simple Demo Component",                           // Description
//      NULL,                                              // PrintHelp
//      create_simpleComponent,                            // Allocator
//      simpleComponent_params,                            // Parameters
//      simpleComponent_ports,                             // Ports
//      COMPONENT_CATEGORY_PROCESSOR,                      // Category
//      NULL                                               // Statistics
//    },


    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        simpleComponent,
        "simpleElementExample",
        "simpleComponent",
        "Simple Demo Component",
        COMPONENT_CATEGORY_PROCESSOR
    )
    
    SST_ELI_DOCUMENT_PARAMS(
        { "workPerCycle", "Count of busy work to do during a clock tick.", NULL},
        { "commFreq", "Approximate frequency of sending an event during a clock tick.", NULL},
        { "commSize", "Size of communication to send.", "16"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
        {"Nlink", "Link to the simpleComponent to the North", { "simpleComponent.simpleComponentEvent", "" } },
        {"Slink", "Link to the simpleComponent to the South", { "simpleComponent.simpleComponentEvent", "" } },
        {"Elink", "Link to the simpleComponent to the East",  { "simpleComponent.simpleComponentEvent", "" } },
        {"Wlink", "Link to the simpleComponent to the West",  { "simpleComponent.simpleComponentEvent", "" } }
    )
    
};

} // namespace SimpleComponent
} // namespace SST

#endif /* _SIMPLECOMPONENT_H */
