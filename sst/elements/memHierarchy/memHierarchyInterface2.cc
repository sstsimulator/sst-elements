// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "memHierarchyInterface.h"


using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;



void MemHierarchyInterface::setup(){}
void MemHierarchyInterface::init(unsigned int _phase){}
void MemHierarchyInterface::finish(){}



MemEvent::id_type MemHierarchyInterface::read(MemEvent* _memEvent){
    return make_pair(0,0);
}


MemEvent::id_type MemHierarchyInterface::read(Addr _addr, int _size, bool _lock){
    return make_pair(0,0);

}

void MemHierarchyInterface::write(MemEvent* _memEvent){



}

void MemHierarchyInterface::write(Addr _addr, std::vector<uint8_t> _data, int _size, bool _release){



}


void MemHierarchyInterface::resendNackedEvents(){


}








