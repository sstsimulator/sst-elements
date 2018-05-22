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

#ifndef _OPALEVENT_H
#define _OPALEVENT_H

namespace SST {
namespace OpalComponent {

class OpalEvent : public SST::Event 
{
public:
    typedef std::vector<char> dataVec;
    OpalEvent() : SST::Event() { }
    dataVec payload;

public:	
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & payload;
    }
    
    ImplementSerializable(SST::OpalComponent::OpalEvent);     
};

} // namespace SimpleComponent
} // namespace SST

#endif /* _SIMPLECOMPONENTEVENT_H */
