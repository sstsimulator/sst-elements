// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
    }
    
    ImplementSerializable(SST::SimpleMessageGeneratorComponent::simpleMessage);     
};

} // namespace SimpleMessageGeneratorComponent
} // namespace SST

#endif /* _SIMPLEMESSAGE_H */
