// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLECOMPONENTEVENT_H
#define _SIMPLECOMPONENTEVENT_H

#include <sst/core/event.h>

namespace SST {
namespace SimpleComponent {

class simpleComponentEvent : public SST::Event {
public:
  typedef std::vector<char> dataVec;
  simpleComponentEvent() : SST::Event() { }
  dataVec payload;

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(payload);
    }
};


}
}

#endif /* _SIMPLECOMPONENTEVENT_H */
