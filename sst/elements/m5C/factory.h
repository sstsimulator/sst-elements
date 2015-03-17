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

#ifndef _factory_h
#define _factory_h

#include <sst/core/params.h>
#include <dll/gem5dll.hh>

using namespace std;

namespace SST {
namespace M5 {

class M5;

class Factory {
  public:
    Factory(M5* m5_object);
    ~Factory();
    Gem5Object_t* createObject(std::string name, std::string type, 
                               SST::Params& object_params);
    
  private:     
    Gem5Object_t* createWrappedObject(std::string, std::string, SST::Params&);
    Gem5Object_t* createDirectObject(std::string, std::string, SST::Params&);
    M5* m_comp;
};

}
}

#endif

