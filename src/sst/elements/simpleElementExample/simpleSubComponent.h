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

};

} // namespace SimpleSubComponent
} // namespace SST



#endif /* _SIMPLESUBCOMPONENT_H */
