// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLECOMPONENTEVENT_H
#define _SIMPLECOMPONENTEVENT_H

namespace SST {
namespace SimpleComponent {

class simpleComponentEvent : public SST::Event 
{
public:
    typedef std::vector<char> dataVec;
    simpleComponentEvent() : SST::Event() { }
    dataVec payload;

public:	
    void serialize_order(SST::Core::Serialization::serializer &ser) {
        Event::serialize_order(ser);
        ser & payload;
    }
    
    ImplementSerializable(SST::SimpleComponent::simpleComponentEvent);     
};

} // namespace SimpleComponent
} // namespace SST

#endif /* _SIMPLECOMPONENTEVENT_H */
