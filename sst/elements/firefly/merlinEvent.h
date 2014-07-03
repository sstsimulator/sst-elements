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


#ifndef COMPONENTS_FIREFLY_MERLINEVENT_H
#define COMPONENTS_FIREFLY_MERLINEVENT_H

namespace SST {
namespace Firefly {


class MerlinFireflyEvent : public Merlin::RtrEvent {

  public:
    uint16_t        seq;
    std::string     buf;

    MerlinFireflyEvent() {}

    MerlinFireflyEvent(const MerlinFireflyEvent *me) :
        Merlin::RtrEvent()
    {
        buf = me->buf;
        seq = me->seq;
    }

    MerlinFireflyEvent(const MerlinFireflyEvent &me) :
        Merlin::RtrEvent()
    {
        buf = me.buf;
        seq = me.seq;
    }

    virtual RtrEvent* clone(void)
    {
        return new MerlinFireflyEvent(*this);
    }

    void setDest( int _dest ) {
        dest = _dest;
    }

    void setSrc( int _src ) {
        src = _src;
    }
    void setPktSize() {
        size_in_bits = buf.size() * 8; 
    }

  private:

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RtrEvent);
        ar & BOOST_SERIALIZATION_NVP(seq);
        ar & BOOST_SERIALIZATION_NVP(buf);
    }
};

}
}

#endif
