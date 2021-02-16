// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_MEM_H
#define COMPONENTS_FIREFLY_MEM_H

#include <sst/core/component.h>
#include <sst/core/link.h>

namespace SST {
namespace Firefly {

typedef std::function<void()> Callback;

class MemCpyReqEvent : public Event {

  public:
    typedef uint64_t Addr;
    MemCpyReqEvent( Callback _callback, int _core, Addr _to,
                            Addr _from, size_t _length )  :
        Event(),
        callback( _callback ),
        core(_core),
        to(_to),
        from(_from),
        length(_length)
    {}

    Callback callback;
    int core;
    Addr to;
    Addr from;
    size_t length;

    NotSerializable(MemCpyReqEvent)
};

class MemReadReqEvent : public Event {

  public:
    typedef uint64_t Addr;
    MemReadReqEvent( Callback _callback, int _core, Addr _addr,
                            size_t _length )  :
        Event(),
        callback( _callback ),
        core(_core),
        addr(_addr),
        length(_length)
    {}

    Callback callback;
    int core;
    Addr addr;
    size_t length;

    NotSerializable(MemReadReqEvent)
};

class MemWriteReqEvent : public Event {

  public:
    typedef uint64_t Addr;
    MemWriteReqEvent( Callback _callback, int _core, Addr _addr,
                            size_t _length )  :
        Event(),
        callback( _callback ),
        core(_core),
        addr(_addr),
        length(_length)
    {}

    Callback callback;
    int core;
    Addr addr;
    size_t length;

    NotSerializable(MemWriteReqEvent)
};

class MemRespEvent : public Event {

  public:
    MemRespEvent( Callback _callback )  :
        Event(),
        callback( _callback )
    {}

    Callback callback;

    NotSerializable(MemRespEvent)
};

class Mem : public SST::Component  {
  public:
    Mem(ComponentId_t id, Params& params );

    ~Mem() {}
    void handleMemEvent( Event* ev, int );

  private:
    std::vector<Link*>          m_links;
};


inline Mem::Mem(ComponentId_t id, Params& params ) :
        Component( id )
{
    int numCores = params.find<int>("numCores", 1 );

    for ( int i = 0; i < numCores; i++ ) {
        std::ostringstream tmp;
        tmp <<  i;

        Event::Handler<Mem,int>* handler =
                new Event::Handler<Mem,int>(
                                this, &Mem::handleMemEvent, i );

        Link* link = configureLink("core" + tmp.str(), "1 ns", handler );
        assert(link);
        m_links.push_back( link );
    }
}

inline void Mem::handleMemEvent( Event* ev, int src ) {
    {
        MemCpyReqEvent* event = dynamic_cast<MemCpyReqEvent*>(ev);

        if ( event ) {
            uint64_t delay = 0;
            if ( event->length) delay = 11;
            m_links[event->core]->send(delay, new MemRespEvent(event->callback) );
            return;
        }
    }

    {
        MemWriteReqEvent* event = dynamic_cast<MemWriteReqEvent*>(ev);

        if ( event ) {
            uint64_t delay = 0;
            if ( event->length) delay = 11;
            m_links[event->core]->send(delay, new MemRespEvent(event->callback) );
            return;
        }
    }

    {
        MemReadReqEvent* event = dynamic_cast<MemReadReqEvent*>(ev);

        if ( event ) {
            uint64_t delay = 0;
            if ( event->length) delay = 11;
            m_links[event->core]->send(delay, new MemRespEvent(event->callback) );
            return;
        }
    }

    delete ev;
}

}
}

#endif
