//
//  memoryInterface.cpp
//  SST
//
//  Created by Caesar De la Paz III on 4/9/14.
//  Copyright (c) 2014 De la Paz, Cesar. All rights reserved.
//

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








