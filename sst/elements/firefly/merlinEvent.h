// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_FIREFLY_MERLINEVENT_H
#define COMPONENTS_FIREFLY_MERLINEVENT_H

#include <sst/core/interfaces/simpleNetwork.h>

namespace SST {
namespace Firefly {


class FireflyNetworkEvent : public Event {

  public:
    uint16_t        seq;
    std::string     buf;
    int             src;

    FireflyNetworkEvent() {}

    FireflyNetworkEvent(const FireflyNetworkEvent *me) :
        Event()
    {
        buf = me->buf;
        seq = me->seq;
        src = me->src;
    }

    FireflyNetworkEvent(const FireflyNetworkEvent &me) :
        Event()
    {
        buf = me.buf;
        seq = me.seq;
        src = me.src;
    }

    virtual Event* clone(void)
    {
        return new FireflyNetworkEvent(*this);
    }

    // void setDest( int _dest ) {
    //     dest = _dest;
    // }

    // void setSrc( int _src ) {
    //     src = _src;
    // }
    // void setPktSize() {
    //     size_in_bits = buf.size() * 8; 
    // }

  private:

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(seq);
        ar & BOOST_SERIALIZATION_NVP(buf);
        ar & BOOST_SERIALIZATION_NVP(src);
    }
};

}
}

#endif
