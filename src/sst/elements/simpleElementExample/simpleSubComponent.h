// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
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

#include<vector>

#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>

namespace SST {
namespace SimpleSubComponent {

/*

  SimpleSubComponent will test the various ways to use SubComponents.
  The SubComponents will be loadable as both named and unnamed
  SubComponets.  When an unname SubComponent is used, it inherits the
  port interface from the BaseComponent that created it.  A named
  SubComponent owns it's own ports and will mask the ports from any
  BaseComponent higher in the call tree.

  Each BaseComponent will have a port(s) that may or may not be used
  depending on the configuration.

  Configurations to be supported:
  
 */


class SubCompInterface : public SST::SubComponent
{
public:
    SubCompInterface(Component *owningComponent) :
        SubComponent(owningComponent)
    { }
    SubCompInterface(ComponentId_t id) :
        SubComponent(id)
    { }
    SubCompInterface(ComponentId_t id, Params& params) :
        SubComponent(id)
    { }
    SubCompInterface(Component* comp, Params& params) :
        SubComponent(comp)
    { }
    virtual ~SubCompInterface() {}
    virtual void clock(SST::Cycle_t) {}


    // typedef std::tuple<int,int> ELI_CONSTRUCTOR_PARAMS;

    // template <class... ARGS>
    // static ELI_CONSTRUCTOR_PARAMS convert(ARGS... args) {
    //     return std::make_tuple<ARGS...>(std::forward<ARGS>(args)...);
    // }

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::SimpleSubComponent::SubCompInterface)
    // SST_ELI_DECLARE_NEW_BASE(SubComponent,SubCompInterface)
    // SST_ELI_NEW_BASE_CTOR(ComponentId_t,Params&)

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        SubCompInterface,
        "simpleElementExample",
        "SubCompInterface",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Default implementation of SubCompInterface",
        SST::SimpleSubComponent::SubCompInterface
    )
    
};

/* Our trivial component */
class SubComponentLoader : public Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(SubComponentLoader,
                               "simpleElementExample",
                               "SubComponentLoader",
                               SST_ELI_ELEMENT_VERSION(1,0,0),
                               "Demonstrates subcomponents",
                               COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"clock", "Clock Rate", "1GHz"},
        {"unnamed_subcomponent", "Unnamed SubComponent to load.  If empty, then a named subcomponent is loaded", ""},
        {"num_subcomps","Number of anonymous SubComponents to load.  Ignored if using name SubComponents.","1"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"totalSent", "# of total messages sent", "", 1},
    )

    // This ports will be used only by unnamed SubComponents
    SST_ELI_DOCUMENT_PORTS(
        {"port%(num_subcomps)d", "Sending or Receiving Port(s)", { "simpleMessageGeneratorComponent.simpleMessage", "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"mySubComp", "Test slot", "SST::SimpleSubComponent::SubCompInterface" }
    )

    SubComponentLoader(ComponentId_t id, SST::Params& params);

private:

    bool tick(SST::Cycle_t);
    std::vector<SubCompInterface*> subComps;
};


/* Our example subcomponents */


class SubCompSlot : public SubCompInterface
{
public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        SubCompSlot,
        "simpleElementExample",
        "SubCompSlot",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Subcomponent which is just a wrapper for the actual SubComponent to be used",
        SST::SimpleSubComponent::SubCompInterface
    )

    
    SST_ELI_DOCUMENT_PARAMS(
        {"sendCount", "Number of Messages to Send", "10"},
        {"unnamed_subcomponent", "Unnamed SubComponent to load.  If empty, then a named subcomponent is loaded", ""},
        {"num_subcomps","Number of anonymous SubComponents to load.  Ignored if using name SubComponents.","1"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    // Only used when loading unnamed SubComponents
    SST_ELI_DOCUMENT_PORTS(
        {"slot_port%(num_subcomps)d", "Port(s) to send or receive on", { "simpleMessageGeneratorComponent.simpleMessage", "" } },
    )
    
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"mySubCompSlot", "Test slot", "SST::SimpleSubComponent::SubCompInterface" }
    )

    
private:    
    std::vector<SubCompInterface*> subComps;
    
public:
    // Legacy API
    SubCompSlot(Component *owningComponent, Params &params);
    // New API
    SubCompSlot(ComponentId_t id, Params& params);
    // Direct load
    SubCompSlot(ComponentId_t id, std::string unnamed_sub);

//SubCompSlot(ComponentId_t id) : SubCompInterface(id) {}
    ~SubCompSlot() {}
    void clock(Cycle_t);

};




class SubCompSender : public SubCompInterface
{
public:

    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        SubCompSender,
        "simpleElementExample",
        "SubCompSender",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Sending Subcomponent",
        SST::SimpleSubComponent::SubCompInterface
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"port_name", "Name of port to connect to", ""},
        {"sendCount", "Number of Messages to Send", "10"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"numSent", "# of msgs sent", "", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"sendPort", "Sending Port", { "simpleMessageGeneratorComponent.simpleMessage", "" } },
    )
    
    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

private:    
    Statistic<uint32_t> *nMsgSent;
    Statistic<uint32_t> *totalMsgSent;
    uint32_t nToSend;
    SST::Link *link;
public:
    // Legacy API
    SubCompSender(Component *owningComponent, Params &params);
    // New API
    SubCompSender(ComponentId_t id, Params &params);
    // Direct API
    SubCompSender(ComponentId_t id, uint32_t nToSend, const std::string& port_name);
    ~SubCompSender() {}
    void clock(Cycle_t);

};


class SubCompReceiver : public SubCompInterface
{

public:

    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        SubCompReceiver,
        "simpleElementExample",
        "SubCompReceiver",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Receiving Subcomponent",
        SST::SimpleSubComponent::SubCompInterface
    )


    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"numRecv", "# of msgs recv", "", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"recvPort", "Receiving Port", { "simpleMessageGeneratorComponent.simpleMessage", "" } },
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

private:

    Statistic<uint32_t> *nMsgReceived;
    SST::Link *link;

    void handleEvent(SST::Event *ev);

public:
    SubCompReceiver(Component *owningComponent, Params &params);
    SubCompReceiver(ComponentId_t id, Params &params);
    SubCompReceiver(ComponentId_t id, std::string port) ;
    ~SubCompReceiver() {}
    void clock(Cycle_t);

};

} // namespace SimpleSubComponent
} // namespace SST




#endif /* _SIMPLESUBCOMPONENT_H */
