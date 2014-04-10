// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MEMNIC_H_
#define _MEMHIERARCHY_MEMNIC_H_

#include <string>
#include <deque>
#include <vector>

#include <sst/core/component.h>
#include <sst/core/debug.h>
#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/memEvent.h>

#include <sst/elements/merlin/linkControl.h>

using namespace SST;
using namespace SST::Interfaces;
using namespace std;

namespace SST {
namespace MemHierarchy {

class MemHierarchyInterface : public Module {
public:

    MemHierarchyInterface(Link* _link, Event::HandlerBase* _readFunctor = NULL);

    MemEvent::id_type read(MemEvent* _memEvent);
    MemEvent::id_type read(Addr _addr, int _size, bool _lock);

    void write(MemEvent* _memEvent);
    void write(Addr _addr, std::vector<uint8_t> _data, int _size, bool _release);
    
    inline void setReadFunctor(Event::HandlerBase* _readFunctor) { readFunctor_ = _readFunctor; }
    void configureLink(Component* _owner, std::string _portName, TimeConverter* _timeBase);

    void clock(void);
    void setup();
    void init(unsigned int _phase);
    void finish();
    
private:
    Output* dbg_;
    Component* owner_;
    Event::HandlerBase* readFunctor_;
    //vector<CacheEvent*> nackBuffer;
    
    void resendNackedEvents();
    
    //void handleIncomingEvents(CacheEvent* _cacheEvent);
};

}}

#endif
