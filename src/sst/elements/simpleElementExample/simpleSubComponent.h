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

#ifndef _SIMPLESUBCOMPONENT_H
#define _SIMPLESUBCOMPONENT_H

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>

namespace SST {
namespace SimpleSubComponent {

class SubCompInterface : public SST::SubComponent
{
public:
    SubCompInterface(Component *owningComponent) :
        SubComponent(owningComponent)
    { }
    virtual ~SubCompInterface() {}
    virtual void clock(SST::Cycle_t) = 0;
};

/* Our trivial component */
class SubComponentLoader : public Component
{
public:
    SubComponentLoader(ComponentId_t id, SST::Params& params);

private:

    bool tick(SST::Cycle_t);
    SubCompInterface* subComp;

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(SubComponentLoader,
                               "simpleElementExample",
                               "SubComponentLoader",
                               "Demonstrates subcomponentst",
                               COMPONENT_CATEGORY_UNCATEGORIZED
    )
    
    SST_ELI_DOCUMENT_PARAMS(
        {"clock", "Clock Rate", "1GHz"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
    )
};


/* Our example subcomponents */
class SubCompSender : public SubCompInterface
{
    Statistic<uint32_t> *nMsgSent;
    uint32_t nToSend;
    SST::Link *link;
public:
    SubCompSender(Component *owningComponent, Params &params);
    ~SubCompSender() {}
    void clock(Cycle_t);

    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
        SubCompSender,
        "simpleElementExample",
        "SubCompSender",
        "Sending Subcomponent",
        "SST::simpleSubComponent::SubCompInterface"
    )
    
    SST_ELI_DOCUMENT_PARAMS(
        {"sendCount", "Number of Messages to Send", "10"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"numSent", "# of msgs sent", "", 1}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"sendPort", "Sending Port", { "simpleMessageGeneratorComponent.simpleMessage", "" } }
    )
    
};


class SubCompReceiver : public SubCompInterface
{
    Statistic<uint32_t> *nMsgReceived;
    SST::Link *link;

    void handleEvent(SST::Event *ev);

public:
    SubCompReceiver(Component *owningComponent, Params &params);
    ~SubCompReceiver() {}
    void clock(Cycle_t);

    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
        SubCompReceiver,
        "simpleElementExample",
        "SubCompReceiver",
        "Receiving Subcomponent",
        "SST::simpleSubComponent::SubCompInterface"
    )
    
    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"numRecv", "# of msgs sent", "", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"recvPort", "Receiving Port", { "simpleMessageGeneratorComponent.simpleMessage", "" } }
    )
};

//static const ElementInfoParam simpleSubComp_SubCompLoader_params[] = {
//    {"clock", "Clock Rate", "1GHz"},
//    {NULL, NULL, NULL}
//};

//static const ElementInfoSubComponentHook simpleSubComp_SubCompLoader_subComps[] = {
//    {"mySubComp", "Subcomponent to do my real work", "SST::SimpleSubComponent::SubCompInterface"},
//    {NULL, NULL, NULL}
//};

//static const ElementInfoParam simpleSubComp_SubCompSender_params[] = {
//    {"sendCount", "Number of Messages to Send", "10"},
//    {NULL, NULL, NULL}
//};
//
//static const ElementInfoStatistic simpleSubComp_SubCompSender_stats[] = {
//    {"numSent", "# of msgs sent", "", 1},
//    {NULL, NULL, NULL, 0}
//};
//
//static const ElementInfoPort simpleSubComp_SubCompSender_ports[] = {
//    {"sendPort", "Sending Port", simpleMessageGeneratorComponent_port_events},
//    {NULL, NULL, NULL}
//};

//static const ElementInfoParam simpleSubComp_SubCompReceiver_params[] = {
//    {NULL, NULL, NULL}
//};
//
//static const ElementInfoStatistic simpleSubComp_SubCompReceiver_stats[] = {
//    {"numRecv", "# of msgs sent", "", 1},
//    {NULL, NULL, NULL, 0}
//};
//
//static const ElementInfoPort simpleSubComp_SubCompReceiver_ports[] = {
//    {"recvPort", "Receiving Port", simpleMessageGeneratorComponent_port_events},
//    {NULL, NULL, NULL}
//};


//static const ElementInfoSubComponent simpleElementSubComponents[] = {
//    {
//        .name = "SubCompSender",
//        .description = "Sending Subcomponent",
//        .printHelp = NULL,
//        .alloc = create_SubCompSender,
//        .params = simpleSubComp_SubCompSender_params,
//        .stats = simpleSubComp_SubCompSender_stats,
//        .provides = "SST::simpleSubComponent::SubCompInterface",
//        .ports = simpleSubComp_SubCompSender_ports,
//        .subComponents = NULL
//    },
//    {
//        .name = "SubCompReceiver",
//        .description = "Receiving Subcomponent",
//        .printHelp = NULL,
//        .alloc = create_SubCompReceiver,
//        .params = simpleSubComp_SubCompReceiver_params,
//        .stats = simpleSubComp_SubCompReceiver_stats,
//        .provides = "SST::simpleSubComponent::SubCompInterface",
//        .ports = simpleSubComp_SubCompReceiver_ports,
//        .subComponents = NULL
//    },
//    {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
//};
//
//static const ElementInfoComponent simpleElementComponents[] = {
//    {
//        .name = "SubComponentLoader",
//        .description = "Demonstrates subcomponents",
//        .printHelp = NULL,
//        .alloc = create_SubComponentLoader,
//        .params = simpleSubComp_SubCompLoader_params,
//        .ports = NULL,
//        .category = COMPONENT_CATEGORY_UNCATEGORIZED,
//        .stats = NULL,
//        .subComponents = simpleSubComp_SubCompLoader_subComps
//    },
   

} // namespace SimpleSubComponent
} // namespace SST




#endif /* _SIMPLESUBCOMPONENT_H */
