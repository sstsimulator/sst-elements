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

#ifndef _SIMPLEMESSAGE_H
#define _SIMPLEMESSAGE_H

namespace SST {
namespace SimpleMessageGeneratorComponent {

class simpleMessage : public SST::Event 
{
public:
    simpleMessage() : SST::Event() { }

public:	
    void serialize_order(SST::Core::Serialization::serializer &ser) {
        Event::serialize_order(ser);
    }
    
    ImplementSerializable(SST::SimpleMessageGeneratorComponent::simpleMessage);     
};

} // namespace SimpleMessageGeneratorComponent
} // namespace SST

#endif /* _SIMPLEMESSAGE_H */
