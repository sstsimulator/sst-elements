
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


#ifndef COMPONENTS_FIREFLY_IOEVENT_H
#define COMPONENTS_FIREFLY_IOEVENT_H

#include <sst/core/sst_types.h>
#include "sst/core/interfaces/stringEvent.h"

#include "ioapi.h"

namespace SST {
namespace Firefly {

class IOEvent : public SST::Interfaces::StringEvent {
  public:
    IOEvent( IO::NodeId _nodeId, const std::string &str ) :
        StringEvent( str ), nodeId( _nodeId )
    { }
    int getNodeId() { return nodeId; }
    void setNodeId( IO::NodeId id) { nodeId = id; }
  private:
    int nodeId;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(StringEvent);
        ar & BOOST_SERIALIZATION_NVP(nodeId);
    }
};

}
}

#endif
