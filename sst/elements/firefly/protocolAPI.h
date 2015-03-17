// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_PROTOCOLAPI_H
#define COMPONENTS_FIREFLY_PROTOCOLAPI_H

#include <sst/core/module.h>

namespace SST {
namespace Firefly {

class Info;
class VirtNic;

class ProtocolAPI : public SST::Module 
{
  public:

    virtual ~ProtocolAPI() {}
    virtual void printStatus( Output& ) {}
    virtual void setup() {};
    virtual void init( Info*, VirtNic* ) = 0;  
    virtual std::string name() = 0;
    virtual void setRetLink(SST::Link* link) { assert(0); } 
};

}
}
#endif
