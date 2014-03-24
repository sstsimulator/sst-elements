// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_LOOPBACK_H
#define COMPONENTS_FIREFLY_LOOPBACK_H

#include <sst/core/component.h>

namespace SST {
namespace Firefly {

class LoopBackEvent : public Event {

  public:
    LoopBackEvent( int _dest ) :
        Event(),
        dest( _dest )
    {
    }
    int dest;
  private:
        friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(dest);
    }
};

class LoopBack : public SST::Component  {
  public:
    LoopBack(ComponentId_t id, Params& params );
    ~LoopBack();
    void handleCoreEvent( Event* ev );

  private:
    std::vector<Link*>          m_links;   
    Event::Handler<LoopBack>*   m_handler;
};

}
}

#endif
