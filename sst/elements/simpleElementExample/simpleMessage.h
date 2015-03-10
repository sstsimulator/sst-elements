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

#ifndef _SIMPLEMESSAGE_H
#define _SIMPLEMESSAGE_H

#include <sst/core/event.h>

namespace SST {
namespace SimpleMessageGeneratorComponent {

class simpleMessage : public SST::Event {
public:
  simpleMessage() : SST::Event() { }

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
    }
};

}
}

#endif /* _SIMPLEMESSAGE_H */
